#include "featureLevelSet.hpp"
#include "levelSetJob.hpp"
#include "multiField.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <thread>
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
    job.start();
    while (!job.finished()) std::this_thread::yield();
    EXPECT_FALSE(job.take().has_value());
}

TEST_F(LevelSetJobTests, MainThreadSteppingProducesSameSurfaceAsSynchronousPath) {
    FeatureLevelSet expected(points, *dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    ASSERT_GT(expected.surface.vertices.size(), 0u);

    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    while (!job.finished()) job.stepOnMainThread(1);
    const std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->surface.vertices, expected.surface.vertices);
}

TEST_F(LevelSetJobTests, BackgroundJobProducesSameSurfaceAsSynchronousPath) {
    FeatureLevelSet expected(points, *dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    LevelSetJob job(points, dataset, 0.5f, {1.f, 0.f, 0.f}, "a");
    job.start();
    while (!job.finished()) std::this_thread::yield();
    const std::optional<FeatureLevelSet> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->surface.vertices, expected.surface.vertices);
}
