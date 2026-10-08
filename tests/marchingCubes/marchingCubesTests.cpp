#include "marchingCubes.hpp"
#include "mc_tables.h"
#include "scalarField.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <sstream>

TEST(MarchingCubesTests, TestNoActiveVertices) {
    ScalarField field{{3, 3, 3}, std::vector<float>(27, 1.f)};
    float isoValue = 0;

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    Surface result = extractSurface(field, isoValue);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    // Assertions
    EXPECT_EQ(result.vertices.size(), 0); // Expect 0 surface vertices
    EXPECT_EQ(result.activeVertices.size(), 0); // Expect 0 active vertices
    EXPECT_EQ(result.inactiveVertices.size(), 0); // Expect 0 inactive vertices
}

TEST(MarchingCubesTests, TestSingleActiveVertices) {
    ScalarField field{{3, 3, 3}, std::vector<float>(27, 1.f)};
    field.values[13] = -1.f;
    float isoValue = 0;

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    Surface result = extractSurface(field, isoValue);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    // Assertions
    EXPECT_EQ(result.vertices.size(), 24); // Expect 32 surface vertices
    EXPECT_EQ(result.activeVertices.size(), 1); // Expect 1 active vertices
    EXPECT_EQ(result.inactiveVertices.size(), 6); // Expect 0 inactive vertices
}

TEST(MarchingCubesTests, SingleActiveVertexSurfaceSurroundsThatVertex) {
    // Extends TestSingleActiveVertices: the active corner must be (1,1,1) and, with
    // isovalue 0 midway between -1 and 1, every surface vertex is an edge midpoint
    // half a cell from it along exactly one axis.
    ScalarField field{{3, 3, 3}, std::vector<float>(27, 1.f)};
    field.values[field.layout.index(1, 1, 1)] = -1.f;
    Surface result = extractSurface(field, 0.f);
    ASSERT_EQ(result.activeVertices.size(), 1u);
    EXPECT_EQ(*result.activeVertices.begin(), glm::vec3(1, 1, 1));
    for (const glm::vec3& v : result.vertices) {
        const glm::vec3 d = glm::abs(v - glm::vec3(1, 1, 1));
        EXPECT_FLOAT_EQ(d.x + d.y + d.z, 0.5f);
        EXPECT_FLOAT_EQ(std::max({d.x, d.y, d.z}), 0.5f);
    }
}

TEST(MarchingCubesTests, TestAllCases) {
    float isoValue = 0;

    for (int caseNum = 1; caseNum < 255; ++caseNum) { // Skipping 0 and 255 as they are tested above
        // Corner i is bit (1 << i), in the same order examineCube walks the cube.
        std::vector<float> values(8);
        for (int corner = 0; corner < 8; ++corner)
            values[corner] = (caseNum & (1 << corner)) ? 1.f : -1.f;
        ScalarField field{{2, 2, 2}, std::move(values)};

        // Redirect std::cout
        std::streambuf* originalCoutBuffer = std::cout.rdbuf();
        std::ostringstream redirectedCout;
        std::cout.rdbuf(redirectedCout.rdbuf());

        Surface result = extractSurface(field, isoValue);

        // Restore std::cout to its original buffer
        std::cout.rdbuf(originalCoutBuffer);

        int expectedTriangles = triangleTable[caseNum][0];
        int generatedTriangles = result.vertices.size()/3;

        // Check the expected number of vertices against the number generated
        EXPECT_EQ(generatedTriangles, expectedTriangles) << "Case " << caseNum << " failed.";
    }
}

TEST(SurfaceTests, InterleavedVerticesArePositionThenNormal) {
    Surface s;
    s.vertices = {{1, 2, 3}, {4, 5, 6}};
    s.normals  = {{0, 0, 1}, {0, 1, 0}};
    EXPECT_EQ(s.interleaved(), (std::vector<float>{1, 2, 3, 0, 0, 1,  4, 5, 6, 0, 1, 0}));
}
