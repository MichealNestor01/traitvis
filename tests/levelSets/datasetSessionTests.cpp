#include "datasetReader.hpp"
#include "datasetSession.hpp"

#include <gtest/gtest.h>
#include <memory>

namespace {

std::shared_ptr<MultiField> loadTestDataset() {
    return std::make_shared<MultiField>(readDataset("tests/parsingData/testDataset/config.txt"));
}

}

TEST(DatasetSessionTests, RemovingADatasetKeepsLevelSetsBuiltFromItAlive) {
    DatasetSession session;
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
