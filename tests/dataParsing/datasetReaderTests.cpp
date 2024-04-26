#include "datasetReader.hpp"
#include "datasetConfig.hpp"
#include "configParser.hpp"
#include "multiField.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

TEST(DatasetReaderTests, TestDataset) {
    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    MultiField result = readDataset("tests/dataParsing/testDataset/config.txt");

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    EXPECT_EQ(result.readError, false);
    EXPECT_EQ(result.name, "testDataset");
    EXPECT_EQ(result.xVals, 2);
    EXPECT_EQ(result.yVals, 2);
    EXPECT_EQ(result.zVals, 2);
    EXPECT_EQ(result.attributeDomain.size(), 1);
    EXPECT_EQ(result.attributeDomain[0].name, "testAttribute");
    EXPECT_EQ(result.attributeDomain[0].bounds.lower, -1);
    EXPECT_EQ(result.attributeDomain[0].bounds.upper, 1);
    for (int i = 0; i < 8; i++) EXPECT_EQ(result.attributeDomain[0].values[i], 0.5);
}