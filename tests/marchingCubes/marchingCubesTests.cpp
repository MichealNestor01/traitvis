#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "marchingCubes.hpp"
#include "grid.hpp"

TEST(MarchingCubesTests, TestNoActiveVertices) {
    // Setup test grid and layout
    GridLayout layout = {3, 3, 3}; 
    std::vector<std::vector<float>> grid = {
        // x layer 1
        {0, 0, 0, 0},  {0, 0, 1, 0},  {0, 0, 2, 0}, 
        {0, 1, 0, 0},  {0, 1, 1, 0},  {0, 1, 2, 0}, 
        {0, 2, 0, 0},  {0, 2, 1, 0},  {0, 2, 2, 0}, 
        // x layer 2
        {1, 0, 0, 0},  {1, 0, 1, 0},  {1, 0, 2, 0}, 
        {1, 1, 0, 0},  {1, 1, 1, 0},  {1, 1, 2, 0}, 
        {1, 2, 0, 0},  {1, 2, 1, 0},  {1, 2, 2, 0}, 
        // x layer 3
        {2, 0, 0, 0},  {2, 0, 1, 0},  {2, 0, 2, 0}, 
        {2, 1, 0, 0},  {2, 1, 1, 0},  {2, 1, 2, 0}, 
        {2, 2, 0, 0},  {2, 2, 1, 0},  {2, 2, 2, 0}, 
    };
    float isoValue = 0.5; 

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
        {0, 0, 0, 0},  {0, 0, 1, 0},  {0, 0, 2, 0}, 
        {0, 1, 0, 0},  {0, 1, 1, 0},  {0, 1, 2, 0}, 
        {0, 2, 0, 0},  {0, 2, 1, 0},  {0, 2, 2, 0}, 
        // x layer 2
        {1, 0, 0, 0},  {1, 0, 1, 0},  {1, 0, 2, 0}, 
        {1, 1, 0, 0},  {1, 1, 1, -1},  {1, 1, 2, 0}, 
        {1, 2, 0, 0},  {1, 2, 1, 0},  {1, 2, 2, 0}, 
        // x layer 3
        {2, 0, 0, 0},  {2, 0, 1, 0},  {2, 0, 2, 0}, 
        {2, 1, 0, 0},  {2, 1, 1, 0},  {2, 1, 2, 0}, 
        {2, 2, 0, 0},  {2, 2, 1, 0},  {2, 2, 2, 0}, 
    };
    float isoValue = -0.5; 

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