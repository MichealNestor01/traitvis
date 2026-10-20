#include "datasetReader.hpp"
#include "distanceField.hpp"
#include "sessionState.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <vector>

namespace {

std::shared_ptr<MultiField> loadTestDataset() {
    return std::make_shared<MultiField>(readDataset("tests/parsingData/testDataset/config.txt"));
}

std::shared_ptr<MultiField> tinyDataset() {
    auto dataset = std::make_shared<MultiField>();
    dataset->xVals = 2;
    dataset->yVals = 2;
    dataset->zVals = 2;
    dataset->strides = {4, 2, 1};
    Attribute attribute;
    attribute.name = "a";
    attribute.bounds = {0.f, 10.f};
    attribute.values = {0.f, 0.5f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f};
    dataset->attributeDomain.push_back(std::move(attribute));
    return dataset;
}

void finishLevelSet(SessionState& session) {
    ASSERT_TRUE(session.activeLevelSet);
    while (session.activeLevelSet && !session.activeLevelSet->finished()) {
        if (!session.activeLevelSet->runsOnWorker()) session.pollLevelSet();
        else std::this_thread::yield();
    }
    if (session.activeLevelSet) session.pollLevelSet();
}

}

TEST(SessionStateTests, RemovingADatasetKeepsLevelSetsBuiltFromItAlive) {
    SessionState session;
    const std::shared_ptr<MultiField> first = loadTestDataset();
    const std::shared_ptr<MultiField> second = loadTestDataset();
    ASSERT_TRUE(first->ok());
    ASSERT_TRUE(second->ok());
    first->name = "kept";
    second->name = "other";
    session.addDataset(first);
    session.addDataset(second);
    session.setActiveDataset(0);

    TraitPoint point;
    point.values.push_back({&session.activeDataset()->attributeDomain[0], 0.5f});
    session.generateLevelSet({point}, 0.5f, {1.f, 0.f, 0.f}, "a");
    session.removeDataset(0);

    EXPECT_EQ(session.datasets.size(), 1u);
    EXPECT_EQ(session.datasets[0]->name, "other");
    ASSERT_EQ(session.levelSets.size(), 1u);
    ASSERT_TRUE(session.levelSets[0].dataset);
    EXPECT_EQ(session.levelSets[0].dataset->name, "kept");
}

TEST(SessionStateTests, GpuDistanceFieldReplacesTheCpuSlabs) {
    SessionState session;
    const std::shared_ptr<MultiField> dataset = tinyDataset();
    session.addDataset(dataset);
    session.attributeSpacePointsBuffer.push_back(TraitPoint{.values = {{&dataset->attributeDomain[0], 5.f}}});

    bool used = false;
    session.gpuDistanceField = [&](std::span<const TraitPoint> traits, const MultiField& source) -> std::optional<ScalarField> {
        used = true;
        EXPECT_EQ(traits.size(), 1u);
        EXPECT_FLOAT_EQ(traits[0].values[0].value, 0.5f);
        ScalarField zeros;
        zeros.layout = {source.xVals, source.yVals, source.zVals};
        zeros.values.assign(static_cast<std::size_t>(zeros.layout.total()), 0.f);
        return zeros;
    };

    session.generateLevelSet(0.25f, {1.f, 0.f, 0.f}, "gpu");
    EXPECT_TRUE(used);
    ASSERT_NO_FATAL_FAILURE(finishLevelSet(session));

    const TraitPoint raw{.values = {{&dataset->attributeDomain[0], 5.f}}};
    const FeatureLevelSet cpu(std::vector<TraitPoint>{raw}, dataset, 0.25f, {1.f, 0.f, 0.f}, "cpu");
    ASSERT_EQ(session.levelSets.size(), 1u);
    EXPECT_TRUE(session.levelSets[0].surface.vertices.empty());
    EXPECT_FALSE(cpu.surface.vertices.empty());
}

TEST(SessionStateTests, ADeclinedGpuFieldFallsBackToTheCpuSlabs) {
    SessionState session;
    const std::shared_ptr<MultiField> dataset = tinyDataset();
    session.addDataset(dataset);
    session.attributeSpacePointsBuffer.push_back(TraitPoint{.values = {{&dataset->attributeDomain[0], 5.f}}});

    bool used = false;
    session.gpuDistanceField = [&](std::span<const TraitPoint>, const MultiField&) -> std::optional<ScalarField> {
        used = true;
        return std::nullopt;
    };

    session.generateLevelSet(0.25f, {1.f, 0.f, 0.f}, "cpu");
    EXPECT_TRUE(used);
    ASSERT_NO_FATAL_FAILURE(finishLevelSet(session));

    FeatureLevelSet cpu(std::vector<TraitPoint>{TraitPoint{.values = {{&dataset->attributeDomain[0], 5.f}}}}, dataset, 0.25f, {1.f, 0.f, 0.f}, "cpu");
    ASSERT_EQ(session.levelSets.size(), 1u);
    EXPECT_EQ(session.levelSets[0].surface.vertices, cpu.surface.vertices);
}
