#include "datasetReader.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <thread>

TEST(DatasetReadJobTests, CancelBeforeStartYieldsNoDataset) {
    DatasetReadJob job("tests/parsingData/testDataset/config.txt");
    job.cancel();
    job.start();
    while (!job.finished()) std::this_thread::yield();
    EXPECT_FALSE(job.take().has_value());
}

TEST(DatasetReadJobTests, MainThreadSteppingReadsTheDataset) {
    DatasetReadJob job("tests/parsingData/testDataset/config.txt");
    while (!job.finished()) job.stepOnMainThread();
    const std::optional<MultiField> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->ok());
    EXPECT_EQ(result->name, "testDataset");
    EXPECT_EQ(result->xVals, 2);
    EXPECT_EQ(result->attributeDomain.size(), 1u);
    EXPECT_EQ(result->attributeDomain[0].values.size(), 8u);
}

TEST(DatasetReadJobTests, BackgroundReadMatchesTheDataset) {
    DatasetReadJob job("tests/parsingData/testDataset/config.txt");
    job.start();
    while (!job.finished()) std::this_thread::yield();
    const std::optional<MultiField> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->ok());
    EXPECT_EQ(result->name, "testDataset");
    EXPECT_FLOAT_EQ(result->attributeDomain[0].values[0], 0.5f);
}
