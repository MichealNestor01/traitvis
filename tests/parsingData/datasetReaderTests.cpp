#include "datasetReader.hpp"
#include "datasetConfig.hpp"
#include "configParser.hpp"
#include "multiField.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

TEST(DatasetReaderTests, TestDataset) {
    MultiField result = readDataset("tests/parsingData/testDataset/config.txt");

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.name, "testDataset");
    EXPECT_EQ(result.xVals, 2);
    EXPECT_EQ(result.yVals, 2);
    EXPECT_EQ(result.zVals, 2);
    EXPECT_EQ(result.attributeDomain.size(), 1);
    EXPECT_EQ(result.attributeDomain[0].name, "testAttribute");
    EXPECT_EQ(result.attributeDomain[0].bounds.lower, 0.5);
    EXPECT_EQ(result.attributeDomain[0].bounds.upper, 0.5);
    for (int i = 0; i < 8; i++) EXPECT_EQ(result.attributeDomain[0].values[i], 0.5);    
}

TEST(DatasetReaderTests, SentinelBecomesNaNAndIsExcludedFromBounds) {
    DatasetDirConfig config;
    config.name = "sentinel";
    config.noData = 1e35f;
    auto dataset = std::make_unique<AttributePerFileDataset>(8, 2, 2, 2, WIDTH_DEPTH_HEIGHT, ROWMAJOR);
    dataset->addFile("unused.bin", "attr", 0.f, 1.f);
    config.dataset = std::move(dataset);

    const std::vector<float> bytes = {0.25f, 1e35f, 0.5f, 0.75f, 0.f, 1.f, 0.5f, 0.5f};
    MultiField result = readDataset(config, [&](const std::filesystem::path&, std::size_t count) {
        EXPECT_EQ(count, bytes.size());
        return bytes;
    });

    ASSERT_TRUE(result.ok());
    const auto& values = result.attributeDomain[0].values;
    EXPECT_TRUE(std::isnan(values[1]));
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.lower, 0.f);
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.upper, 1.f);
    EXPECT_FLOAT_EQ(values[0], 0.25f);
    EXPECT_FLOAT_EQ(values[3], 0.75f);
}

TEST(DatasetReaderTests, MissingConfigReportsReadError) {
    MultiField result = readDataset("tests/parsingData/does-not-exist/config.txt");
    EXPECT_EQ(result.error, "Failed to open \"tests/parsingData/does-not-exist/config.txt\"\n");
}

TEST(DatasetReaderTests, WithoutNoDataTheSentinelValueIsKept) {
    MultiField result = readDataset("tests/parsingData/sentinelDataset/config_without_nodata.txt");

    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(std::isnan(result.attributeDomain[0].values[1]));
    EXPECT_FLOAT_EQ(result.attributeDomain[0].values[1], 1e35f);
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.lower, 0.f);
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.upper, 1.f);
}