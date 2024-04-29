#include "fileReader.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>

TEST(FileReaderTests, TestBrickOfFloatFiles) {
    int totalTestBinaryFiles = 20;
    for (int i = 0; i < totalTestBinaryFiles; ++i) {
        std::ostringstream oss;
        oss << "tests/parsingData/testBinaryFiles/binaryFile" << i << ".bin";
        std::vector<float> result = readFloatBinaryFile(oss.str(), pow(2,i), false);
        for (int j = 0; j < pow(2,i); ++j) {
            // test all items in the array are equal to their index
            EXPECT_EQ(result[j], (float)j) << "Test failed for " << oss.str() << " at index " << j << " expected " << j << " got " << result[j];
        }
    }
}