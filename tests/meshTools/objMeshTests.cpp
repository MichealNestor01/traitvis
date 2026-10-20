#include "objMesh.hpp"

#include <gtest/gtest.h>
#include <sstream>

namespace {

Surface singleTriangle() {
    Surface surface;
    surface.vertices = {{0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}};
    surface.normals = {{0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}};
    return surface;
}

}

TEST(ObjMeshTests, SingleTriangleProducesExactText) {
    std::ostringstream out;
    writeOBJ(singleTriangle(), out);
    EXPECT_EQ(out.str(),
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vn 0 0 1\nvn 0 0 1\nvn 0 0 1\n"
        "f 1//1 2//2 3//3\n");
}

TEST(ObjMeshTests, RoundTripKeepsPositionsAndNormals) {
    Surface surface = singleTriangle();
    surface.vertices.push_back({2.f, 0.f, 0.f});
    surface.vertices.push_back({3.f, 0.f, 0.f});
    surface.vertices.push_back({2.f, 1.f, 0.f});
    surface.normals.push_back({0.f, 1.f, 0.f});
    surface.normals.push_back({0.f, 1.f, 0.f});
    surface.normals.push_back({0.f, 1.f, 0.f});

    std::stringstream out;
    writeOBJ(surface, out);
    const ObjMesh mesh = readOBJ(out);
    ASSERT_TRUE(mesh.ok());
    EXPECT_EQ(mesh.surface.vertices, surface.vertices);
    EXPECT_EQ(mesh.surface.normals, surface.normals);
}

TEST(ObjMeshTests, TextureCoordinatesAreSkipped) {
    std::istringstream in(
        "# comment\n"
        "o triangle\n"
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 0 1\n"
        "vn 0 0 1\nvn 0 0 1\nvn 0 0 1\n"
        "f 1/1/1 2/2/2 3/3/3\n");
    const ObjMesh mesh = readOBJ(in);
    ASSERT_TRUE(mesh.ok());
    EXPECT_EQ(mesh.surface.vertices, singleTriangle().vertices);
    EXPECT_EQ(mesh.surface.normals, singleTriangle().normals);
}

TEST(ObjMeshTests, QuadBecomesTwoTriangles) {
    std::istringstream in(
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
        "f 1 2 3 4\n");
    const ObjMesh mesh = readOBJ(in);
    ASSERT_TRUE(mesh.ok());
    const std::vector<glm::vec3> expected = {
        {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {1.f, 1.f, 0.f},
        {0.f, 0.f, 0.f}, {1.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
    };
    EXPECT_EQ(mesh.surface.vertices, expected);
    ASSERT_EQ(mesh.surface.normals.size(), expected.size());
    for (const glm::vec3& normal : mesh.surface.normals)
        EXPECT_EQ(normal, glm::vec3(0.f, 0.f, 1.f));
}

TEST(ObjMeshTests, IndexPastTheEndReportsAnError) {
    std::istringstream vertices("v 0 0 0\nf 1 2 3\n");
    const ObjMesh missingVertex = readOBJ(vertices);
    EXPECT_FALSE(missingVertex.ok());
    EXPECT_TRUE(missingVertex.error.find("vertices") != std::string::npos);
    EXPECT_TRUE(missingVertex.surface.vertices.empty());

    std::istringstream normals(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vn 0 0 1\n"
        "f 1//1 2//1 3//2\n");
    const ObjMesh missingNormal = readOBJ(normals);
    EXPECT_FALSE(missingNormal.ok());
    EXPECT_TRUE(missingNormal.error.find("normals") != std::string::npos);
    EXPECT_TRUE(missingNormal.surface.vertices.empty());
}

TEST(ObjMeshTests, FaceWithFewerThanThreeCornersReportsAnError) {
    std::istringstream in("v 0 0 0\nv 1 0 0\nf 1 2\n");
    const ObjMesh mesh = readOBJ(in);
    EXPECT_FALSE(mesh.ok());
    EXPECT_TRUE(mesh.surface.vertices.empty());
}

TEST(ObjMeshTests, NegativeIndexUsesTheLastVertex) {
    std::istringstream in(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vn 0 0 1\n"
        "f -3//-1 -2//-1 -1//-1\n");
    const ObjMesh mesh = readOBJ(in);
    ASSERT_TRUE(mesh.ok());
    EXPECT_EQ(mesh.surface.vertices, singleTriangle().vertices);
    EXPECT_EQ(mesh.surface.normals, singleTriangle().normals);
}

TEST(ObjMeshTests, EmptyFileIsAnEmptySurface) {
    std::istringstream in("");
    const ObjMesh mesh = readOBJ(in);
    EXPECT_TRUE(mesh.ok());
    EXPECT_TRUE(mesh.surface.vertices.empty());
    EXPECT_TRUE(mesh.surface.normals.empty());
}
