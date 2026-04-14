#include "datasetConfig.hpp"
#include "configParser.hpp"
#include "testUtils.hpp"

#include <gtest/gtest.h>

TEST(DatasetConfigTests, ParseConfigReturnsValidConfig) {
    StreamRedirect redirect(std::cerr);
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");

    EXPECT_FALSE(config.parseError);
    EXPECT_EQ(config.name, "testDataset");
    EXPECT_NE(config.dataset, nullptr);
}

TEST(DatasetConfigTests, ConfigCanBeMovedWithoutCrash) {
    // before using unique_ptr. I didn't have move constructors and assignment operators.
    // which could result in a double free error, when the copy constructor was fallen back on.
    StreamRedirect redirect(std::cerr);
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");
    ASSERT_FALSE(config.parseError);

    DatasetDirConfig moved = std::move(config);
    EXPECT_FALSE(moved.parseError);
    EXPECT_EQ(moved.name, "testDataset");
}

TEST(DatasetConfigTests, ParsedConfigDatasetIsAccessible) {
    StreamRedirect redirect(std::cerr);
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");
    ASSERT_FALSE(config.parseError);

    EXPECT_EQ(config.dataset->format, ATTRIBUTEPERFILE);
    EXPECT_EQ(config.dataset->xVals, 2);
    EXPECT_EQ(config.dataset->yVals, 2);
    EXPECT_EQ(config.dataset->zVals, 2);
}