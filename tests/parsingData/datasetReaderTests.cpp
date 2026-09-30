#include "datasetReader.hpp"
#include "datasetConfig.hpp"
#include "configParser.hpp"
#include "multiField.hpp"

#include "testUtils.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

TEST(DatasetReaderTests, TestDataset) {
    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    MultiField result = readDataset("tests/parsingData/testDataset/config.txt");

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    EXPECT_EQ(result.readError, false);
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
    StreamRedirect redirect(std::cout);
    MultiField result = readDataset("tests/parsingData/sentinelDataset/config.txt");

    ASSERT_FALSE(result.readError);
    const auto& values = result.attributeDomain[0].values;
    EXPECT_TRUE(std::isnan(values[1]));
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.lower, 0.f);
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.upper, 1.f);
    EXPECT_FLOAT_EQ(values[0], 0.25f);
    EXPECT_FLOAT_EQ(values[3], 0.75f);
}

TEST(DatasetReaderTests, WithoutNoDataTheSentinelValueIsKept) {
    StreamRedirect redirect(std::cout);
    MultiField result = readDataset("tests/parsingData/sentinelDataset/config_without_nodata.txt");

    ASSERT_FALSE(result.readError);
    EXPECT_FALSE(std::isnan(result.attributeDomain[0].values[1]));
    EXPECT_FLOAT_EQ(result.attributeDomain[0].values[1], 1e35f);
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.lower, 0.f);
    EXPECT_FLOAT_EQ(result.attributeDomain[0].bounds.upper, 1.f);
}