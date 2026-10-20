#include "datasetConfig.hpp"
#include "configParser.hpp"

#include <gtest/gtest.h>

TEST(DatasetConfigTests, ParseConfigReturnsValidConfig) {
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");

    EXPECT_TRUE(config.ok());
    EXPECT_EQ(config.name, "testDataset");
    EXPECT_NE(config.dataset, nullptr);
}

TEST(DatasetConfigTests, ConfigCanBeMovedWithoutCrash) {
    // before using unique_ptr. I didn't have move constructors and assignment operators.
    // which could result in a double free error, when the copy constructor was fallen back on.
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");
    ASSERT_TRUE(config.ok());

    DatasetDirConfig moved = std::move(config);
    EXPECT_TRUE(moved.ok());
    EXPECT_EQ(moved.name, "testDataset");
}

TEST(DatasetConfigTests, ParsedConfigDatasetIsAccessible) {
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");
    ASSERT_TRUE(config.ok());

    EXPECT_EQ(config.dataset->format, ATTRIBUTEPERFILE);
    EXPECT_EQ(config.dataset->xVals, 2);
    EXPECT_EQ(config.dataset->yVals, 2);
    EXPECT_EQ(config.dataset->zVals, 2);
    EXPECT_FALSE(config.noData.has_value());
}

TEST(DatasetConfigTests, OptionalNoDataIsParsedAsFloat) {
    DatasetDirConfig config = parseConfig("tests/parsingData/sentinelDataset/config.txt");
    ASSERT_TRUE(config.ok());
    ASSERT_TRUE(config.noData.has_value());
    EXPECT_FLOAT_EQ(*config.noData, 1e35f);
}