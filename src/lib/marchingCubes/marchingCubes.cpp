#include "marchingCubes.hpp"
#include "mc_tables.h"

#include <glm.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <fstream>
#include <algorithm>

namespace {

float partial(const ScalarField& field, glm::ivec3 p, int axis) {
    const int n = axis == 0 ? field.layout.x : axis == 1 ? field.layout.y : field.layout.z;
    const int i = axis == 0 ? p.x : axis == 1 ? p.y : p.z;
    auto at = [&](int index) {
        glm::ivec3 q = p;
        if (axis == 0) q.x = index;
        else if (axis == 1) q.y = index;
        else q.z = index;
        return field.values[field.layout.index(q.x, q.y, q.z)];
    };
    if (n < 2) return 0.f;
    // Central difference in the interior. One cell, one-sided, on the border.
    if (i <= 0) return at(1) - at(0);
    if (i >= n - 1) return at(n - 1) - at(n - 2);
    return (at(i + 1) - at(i - 1)) * 0.5f;
}

glm::vec3 gradientAt(const ScalarField& field, glm::ivec3 p) {
    return {partial(field, p, 0), partial(field, p, 1), partial(field, p, 2)};
}

glm::vec3 faceNormal(const Surface& triangles) {
    const glm::vec3 vertex1 = triangles.vertices[triangles.vertices.size() - 3];
    const glm::vec3 vertex2 = triangles.vertices[triangles.vertices.size() - 2];
    const glm::vec3 vertex3 = triangles.vertices[triangles.vertices.size() - 1];
    return glm::normalize(glm::cross(vertex2 - vertex1, vertex3 - vertex1));
}

}

std::vector<float> Surface::interleaved() const {
    std::vector<float> positionsAndNormals(vertices.size() * 6);
    for (std::size_t vertIndex = 0; vertIndex < vertices.size(); ++vertIndex) {
        positionsAndNormals[vertIndex * 6]     = vertices[vertIndex].x;
        positionsAndNormals[vertIndex * 6 + 1] = vertices[vertIndex].y;
        positionsAndNormals[vertIndex * 6 + 2] = vertices[vertIndex].z;
        positionsAndNormals[vertIndex * 6 + 3] = normals[vertIndex].x;
        positionsAndNormals[vertIndex * 6 + 4] = normals[vertIndex].y;
        positionsAndNormals[vertIndex * 6 + 5] = normals[vertIndex].z;
    }
    return positionsAndNormals;
}

void examineCube(Surface &triangles, const ScalarField &field, int vertex0, float isoValue, NormalMode normals) {
    const GridLayout &layout = field.layout;
    int vertices[8];
    // find the index in the coordinate grid of each vertex of the cube
    vertices[0] = vertex0;
    vertices[1] = vertex0 + 1;
    vertices[2] = vertex0 + layout.z;
    vertices[3] = vertex0 + layout.z + 1;
    vertices[4] = vertex0 + (layout.z * layout.y);
    vertices[5] = vertex0 + (layout.z * layout.y) + 1;
    vertices[6] = vertex0 + (layout.z * layout.y) + layout.z;
    vertices[7] = vertex0 + (layout.z * layout.y) + layout.z + 1;

    // find the case this cube matches with
    int cubeIndex = 0;
    if (field.values[vertices[0]] > isoValue) cubeIndex |= 1;
    if (field.values[vertices[1]] > isoValue) cubeIndex |= 2;
    if (field.values[vertices[2]] > isoValue) cubeIndex |= 4;
    if (field.values[vertices[3]] > isoValue) cubeIndex |= 8;
    if (field.values[vertices[4]] > isoValue) cubeIndex |= 16;
    if (field.values[vertices[5]] > isoValue) cubeIndex |= 32;
    if (field.values[vertices[6]] > isoValue) cubeIndex |= 64;
    if (field.values[vertices[7]] > isoValue) cubeIndex |= 128;

    // No triangles found
    if (cubeIndex == 0 or cubeIndex == 255) return;

    int *matchingCase = triangleTable[cubeIndex];
    // loop through the edges bisected in the matching case
    for (int triIndex = 0; triIndex < matchingCase[0]; ++triIndex) {
        for (int vertIndex = 1; vertIndex <= 3; ++vertIndex) {
            // find the midpoint between the two endpoints of the current edge
            // and add that to the list of vertices
            int edge = matchingCase[triIndex*3+vertIndex];
            int *endPoints = edgeTable[edge];
            int corner0 = vertices[endPoints[0]];
            int corner1 = vertices[endPoints[1]];
            float value0 = field.values[corner0];
            float value1 = field.values[corner1];
            glm::vec3 endPoint0 = glm::vec3(field.layout.coords(corner0));
            glm::vec3 endPoint1 = glm::vec3(field.layout.coords(corner1));
            float isoDistance = (isoValue - value0) / (value1 - value0);
            glm::vec3 vertex = endPoint0 + isoDistance * (endPoint1 - endPoint0);
            triangles.vertices.push_back(vertex);
            if (normals == NormalMode::Gradient) {
                const glm::vec3 n = glm::mix(
                    gradientAt(field, field.layout.coords(corner0)),
                    gradientAt(field, field.layout.coords(corner1)),
                    isoDistance);
                triangles.normals.push_back(n);
            }
            if (value0 < isoValue) {
                triangles.activeVertices.insert(endPoint0);
                triangles.inactiveVertices.insert(endPoint1);
            } else {
                triangles.inactiveVertices.insert(endPoint0);
                triangles.activeVertices.insert(endPoint1);
            }
        }
        const glm::vec3 normal = faceNormal(triangles);
        if (normals == NormalMode::Flat) {
            triangles.normals.push_back(normal);
            triangles.normals.push_back(normal);
            triangles.normals.push_back(normal);
        } else {
            for (std::size_t i = triangles.normals.size() - 3; i < triangles.normals.size(); ++i) {
                triangles.normals[i] = glm::dot(triangles.normals[i], triangles.normals[i]) > 0.f
                    ? glm::normalize(triangles.normals[i])
                    : normal;
            }
        }
    }
}

Surface extractSurface(const ScalarField &field, float isoValue, NormalMode normals) {
    Surface surface;
    const GridLayout &layout = field.layout;

    for (int x = 0; x < layout.x - 1; ++x) {
    for (int y = 0; y < layout.y - 1; ++y) {
    for (int z = 0; z < layout.z - 1; ++z) {
        examineCube(surface, field, layout.index(x, y, z), isoValue, normals);
    }}}

    return surface;
}