#include "fileReader.hpp"
#include "testUtils.hpp"

#include <gtest/gtest.h>
#include <cstddef>
#include <sstream>
#include <vector>

TEST(FileReaderTests, TestBrickOfFloatFiles) {
    int totalTestBinaryFiles = 20;
    for (int i = 0; i < totalTestBinaryFiles; ++i) {
        std::ostringstream oss;
        oss << "tests/parsingData/testBinaryFiles/binaryFile" << i << ".bin";
        const std::size_t expected = std::size_t{1} << i;
        std::vector<float> result = readFloatBinaryFile(oss.str(), expected, false);
        ASSERT_EQ(result.size(), expected);
        for (std::size_t j = 0; j < expected; ++j) {
            // test all items in the array are equal to their index
            EXPECT_EQ(result[j], static_cast<float>(j)) << "Test failed for " << oss.str() << " at index " << j << " expected " << j << " got " << result[j];
        }
    }
}

TEST(FileReaderTests, TruncatedFileReturnsEmpty) {
    // binaryFile0.bin has 2 floats; ask for 1000
    std::vector<float> result = readFloatBinaryFile("tests/parsingData/testBinaryFiles/binaryFile0.bin", 1000, false);
    EXPECT_TRUE(result.empty());
}

TEST(FileReaderTests, MissingFileReturnsEmpty) {
    StreamRedirect cerrRedirect(std::cerr);
    std::vector<float> result = readFloatBinaryFile("tests/parsingData/does-not-exist.bin", 1, false);
    EXPECT_TRUE(result.empty());
}

TEST(FileReaderTests, ReadingFewerFloatsThanPresentSucceeds) {
    // ask for 1 of 2, expect {0.f}
    std::vector<float> result = readFloatBinaryFile("tests/parsingData/testBinaryFiles/binaryFile0.bin", 1, false);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0], 0.f);
}
