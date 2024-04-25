#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include "marchingCubes.hpp"
#include "grid.hpp"

TEST(MarchingCubesTests, TestNoActiveVertices) {
    // Setup test grid and layout
    GridLayout layout = {10, 10, 10}; // Example dimensions
    float** grid = createGrid(layout, {});
    float isoValue = 0.5; // Example isoValue

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    // call the function being tested
    Surface result = extractSurface(grid, layout, isoValue);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);
    
    // Assertions
    EXPECT_EQ(result.vertices.size(), 0); // Expect more than 0 vertices
    EXPECT_EQ(result.activeVertices.size(), 0); // Check active vertices
}