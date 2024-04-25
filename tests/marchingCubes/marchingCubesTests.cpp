#include "marchingCubes.hpp"
#include "mc_tables.h"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

TEST(MarchingCubesTests, TestNoActiveVertices) {
    // Setup test grid and layout
    GridLayout layout = {3, 3, 3}; 
    std::vector<std::vector<float>> grid = {
        // x layer 1
        {0, 0, 0, 1},  {0, 0, 1, 1},  {0, 0, 2, 1}, 
        {0, 1, 0, 1},  {0, 1, 1, 1},  {0, 1, 2, 1}, 
        {0, 2, 0, 1},  {0, 2, 1, 1},  {0, 2, 2, 1}, 
        // x layer 2
        {1, 0, 0, 1},  {1, 0, 1, 1},  {1, 0, 2, 1}, 
        {1, 1, 0, 1},  {1, 1, 1, 1},  {1, 1, 2, 1}, 
        {1, 2, 0, 1},  {1, 2, 1, 1},  {1, 2, 2, 1}, 
        // x layer 3
        {2, 0, 0, 1},  {2, 0, 1, 1},  {2, 0, 2, 1}, 
        {2, 1, 0, 1},  {2, 1, 1, 1},  {2, 1, 2, 1}, 
        {2, 2, 0, 1},  {2, 2, 1, 1},  {2, 2, 2, 1}, 
    };
    float isoValue = 0; 

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    // call the function being tested
    Surface result = extractSurface(grid, layout, isoValue);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);
    
    // Assertions
    EXPECT_EQ(result.vertices.size(), 0); // Expect 0 surface vertices
    EXPECT_EQ(result.activeVertices.size(), 0); // Expect 0 active vertices
    EXPECT_EQ(result.inactiveVertices.size(), 0); // Expect 0 inactive vertices
}

TEST(MarchingCubesTests, TestSingleActiveVertices) {
    // Setup test grid and layout
    GridLayout layout = {3, 3, 3}; 
    std::vector<std::vector<float>> grid = {
        // x layer 1
        {0, 0, 0, 1},  {0, 0, 1, 1},  {0, 0, 2, 1}, 
        {0, 1, 0, 1},  {0, 1, 1, 1},  {0, 1, 2, 1}, 
        {0, 2, 0, 1},  {0, 2, 1, 1},  {0, 2, 2, 1}, 
        // x layer 2
        {1, 0, 0, 1},  {1, 0, 1, 1},  {1, 0, 2, 1}, 
        {1, 1, 0, 1},  {1, 1, 1, -1},  {1, 1, 2, 1}, 
        {1, 2, 0, 1},  {1, 2, 1, 1},  {1, 2, 2, 1}, 
        // x layer 3
        {2, 0, 0, 1},  {2, 0, 1, 1},  {2, 0, 2, 1}, 
        {2, 1, 0, 1},  {2, 1, 1, 1},  {2, 1, 2, 1}, 
        {2, 2, 0, 1},  {2, 2, 1, 1},  {2, 2, 2, 1}, 
    };
    float isoValue = 0; 

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    // call the function being tested
    Surface result = extractSurface(grid, layout, isoValue);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);
    
    // Assertions
    EXPECT_EQ(result.vertices.size(), 24); // Expect 32 surface vertices
    EXPECT_EQ(result.activeVertices.size(), 1); // Expect 1 active vertices
    EXPECT_EQ(result.inactiveVertices.size(), 6); // Expect 0 inactive vertices
}

TEST(MarchingCubesTests, TestAllCases) {
    // Define grid layout
    GridLayout layout = {2, 2, 2};
    float isoValue = 0; 

    for (int caseNum = 1; caseNum < 255; ++caseNum) { // Skipping 0 and 255 as they are tested above
        // Setup test grid based on the current case number
        std::vector<std::vector<float>> grid = {
            // Layer 1
            {0, 0, 0, static_cast<float>((caseNum & 1) ? 1 : -1)},
            {0, 0, 1, static_cast<float>((caseNum & 2) ? 1 : -1)},
            {0, 1, 0, static_cast<float>((caseNum & 4) ? 1 : -1)},
            {0, 1, 1, static_cast<float>((caseNum & 8) ? 1 : -1)},
            // Layer 2
            {1, 0, 0, static_cast<float>((caseNum & 16) ? 1 : -1)},
            {1, 0, 1, static_cast<float>((caseNum & 32) ? 1 : -1)},
            {1, 1, 0, static_cast<float>((caseNum & 64) ? 1 : -1)},
            {1, 1, 1, static_cast<float>((caseNum & 128) ? 1 : -1)}
        };

        // Redirect std::cout
        std::streambuf* originalCoutBuffer = std::cout.rdbuf();
        std::ostringstream redirectedCout;
        std::cout.rdbuf(redirectedCout.rdbuf());

        // Call the function being tested
        Surface result = extractSurface(grid, layout, isoValue);

        // Restore std::cout to its original buffer
        std::cout.rdbuf(originalCoutBuffer);

        int expectedTriangles = triangleTable[caseNum][0];
        int generatedTriangles = result.vertices.size()/3;

        // Check the expected number of vertices against the number generated
        EXPECT_EQ(generatedTriangles, expectedTriangles) << "Case " << caseNum << " failed.";
    }
}