#include "shaderSource.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

TEST(ReadShaderSourceTests, MissingFile_ThrowsRuntimeError) {
    const std::string path = "tests/shaderTools/does-not-exist.vert";

    try {
        readShaderSource(path);
        FAIL() << "Expected std::runtime_error for missing file";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find(path), std::string::npos);
    }
}

TEST(ReadShaderSourceTests, ExistingFile_ReturnsContents) {
    const std::string contents = readShaderSource("assets/colours.vert");

    EXPECT_GT(contents.size(), 0u);
    EXPECT_NE(contents.find("void main"), std::string::npos);
}
