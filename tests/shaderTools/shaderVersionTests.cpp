#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

TEST(ShaderVersionTests, StageShadersCarryNoVersionDirective) {
    int seen = 0;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("assets")) {
        const std::string extension = entry.path().extension().string();
        if (extension != ".vert" && extension != ".frag") continue;
        ++seen;
        std::ifstream input(entry.path());
        ASSERT_TRUE(input) << entry.path();
        std::string firstLine;
        std::getline(input, firstLine);
        EXPECT_NE(firstLine.rfind("#version", 0), 0u) << entry.path();
    }
    EXPECT_EQ(seen, 5);
}

TEST(ShaderVersionTests, ComputeShaderKeepsDesktopVersion) {
    std::ifstream input("assets/distanceField.comp");
    ASSERT_TRUE(input);
    std::string firstLine;
    std::getline(input, firstLine);
    EXPECT_EQ(firstLine, "#version 430 core");
}
