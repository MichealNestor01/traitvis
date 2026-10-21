#include "datasetReader.hpp"
#include "waitForJob.hpp"

#include <gtest/gtest.h>
#include <filesystem>
#include <optional>
#include <vector>

TEST(DatasetReadJobTests, CancelBeforeStartYieldsNoDataset) {
    DatasetReadJob job("tests/parsingData/testDataset/config.txt");
    job.cancel();
    waitForJob(job, [&] { job.stepOnMainThread(); });
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

TEST(DatasetReadJobTests, ProviderSuppliesAttributeBytes) {
    const std::vector<float> bytes(8, 0.25f);
    DatasetReadJob job("tests/parsingData/testDataset/config.txt", [&](const std::filesystem::path&, std::size_t count) {
        EXPECT_EQ(count, bytes.size());
        return bytes;
    });
    while (!job.finished()) job.stepOnMainThread();
    const std::optional<MultiField> result = job.take();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->ok());
    EXPECT_FLOAT_EQ(result->attributeDomain[0].values[0], 0.25f);
}

TEST(DatasetReadJobTests, SelectedFileIsTheOnlyAttributeRead) {
    DatasetReadJob missing("tests/parsingData/testDataset/config.txt", {}, {"does-not-exist.bin"});
    while (!missing.finished()) missing.stepOnMainThread();
    const std::optional<MultiField> rejected = missing.take();
    ASSERT_TRUE(rejected);
    EXPECT_FALSE(rejected->ok());

    DatasetReadJob chosen("tests/parsingData/testDataset/config.txt", {}, {"test.bin"});
    while (!chosen.finished()) chosen.stepOnMainThread();
    const std::optional<MultiField> result = chosen.take();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->ok());
    EXPECT_EQ(result->attributeDomain.size(), 1u);
    EXPECT_EQ(result->attributeDomain[0].name, "testAttribute");
}

TEST(DatasetReadJobTests, BackgroundReadMatchesTheDataset) {
    DatasetReadJob job("tests/parsingData/testDataset/config.txt");
    waitForJob(job, [&] { job.stepOnMainThread(); });
    const std::optional<MultiField> result = job.take();
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->ok());
    EXPECT_EQ(result->name, "testDataset");
    EXPECT_FLOAT_EQ(result->attributeDomain[0].values[0], 0.5f);
}
