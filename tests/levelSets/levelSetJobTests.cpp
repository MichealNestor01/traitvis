#include "featureLevelSet.hpp"
#include "levelSetJob.hpp"
#include "multiField.hpp"
#include "waitForJob.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

class LevelSetJobTests : public ::testing::Test {
protected:
    std::shared_ptr<MultiField> dataset = std::make_shared<MultiField>();
    std::vector<TraitPoint> points;

    void SetUp() override {
        dataset->name = "job";
        dataset->xVals = 4;
        dataset->yVals = 4;
        dataset->zVals = 4;
        dataset->strides = {1, 4, 16};
        Attribute attribute;
        attribute.name = "a";
        attribute.bounds = {0.f, 1.f};
        attribute.values.assign(64, 0.f);
        attribute.values[1 + 4 * 1 + 16 * 1] = 1.f;
        dataset->attributeDomain.push_back(std::move(attribute));
        points.push_back({.values = {{&dataset->attributeDomain[0], 1.f}}});
    }
};

TEST_F(LevelSetJobTests, CancelBeforeStartYieldsNoResult) {
    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    job.cancel();
    waitForJob(job, [&] { job.stepOnMainThread(1); });
    EXPECT_FALSE(job.take().has_value());
}

TEST_F(LevelSetJobTests, MainThreadSteppingProducesSameSurfaceAsSynchronousPath) {
    FeatureLevelSet expected(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    ASSERT_GT(expected.surface.vertices.size(), 0u);

    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    while (!job.finished()) job.stepOnMainThread(1);
    const std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->surface.vertices, expected.surface.vertices);
}

TEST_F(LevelSetJobTests, KeptFieldReslicesWithoutChangingSamples) {
    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a", true);
    while (!job.finished()) job.stepOnMainThread(1);
    std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->distanceField);
    const std::vector<float> samples = result->distanceField->values;
    const std::vector<glm::vec3> first = result->surface.vertices;
    ASSERT_FALSE(first.empty());

    ASSERT_TRUE(result->reslice(0.9f));
    EXPECT_EQ(result->distanceField->values, samples);
    EXPECT_NE(result->surface.vertices, first);
    EXPECT_FLOAT_EQ(result->isoValue, 0.9f);
}

TEST_F(LevelSetJobTests, FieldIsDiscardedUnlessKept) {
    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    while (!job.finished()) job.stepOnMainThread(1);
    std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_FALSE(result->distanceField);
    EXPECT_FALSE(result->reslice(0.9f));
}

TEST_F(LevelSetJobTests, PrecomputedFieldSkipsTheSlabs) {
    ScalarField field = generateDistanceField(points, *dataset);
    const std::vector<float> samples = field.values;
    FeatureLevelSet expected(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a", true);

    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a", true, std::move(field));
    EXPECT_FLOAT_EQ(job.progress(), 1.f);
    EXPECT_FALSE(job.finished());
    job.stepOnMainThread(1);
    ASSERT_TRUE(job.finished());

    std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->distanceField);
    EXPECT_EQ(result->distanceField->values, samples);
    EXPECT_EQ(result->surface.vertices, expected.surface.vertices);
}

TEST_F(LevelSetJobTests, BackgroundJobProducesSameSurfaceAsSynchronousPath) {
    FeatureLevelSet expected(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    waitForJob(job, [&] { job.stepOnMainThread(1); });
    const std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->surface.vertices, expected.surface.vertices);
}
