#include "marchingCubes.hpp"
#include "mc_tables.h"

#include <glm.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <fstream>
#include <algorithm>

// function to use calculate face normals, not needed anymore
void Triangles::computeNormals() {
    for (int triIndex = 0; triIndex < vertices.size(); triIndex+=3) {
        glm::vec3 U = vertices.at(triIndex + 1) - vertices.at(triIndex);
        glm::vec3 V = vertices.at(triIndex + 2) - vertices.at(triIndex);
        normals.push_back(glm::normalize(glm::cross(U, V)));
    }
}

std::vector<float> Triangles::getVertices() {
    std::vector<float> verticesWithNormals(vertices.size() * 6);
    for (int vertIndex = 0; vertIndex < vertices.size(); ++vertIndex) {
        verticesWithNormals[vertIndex*6] = vertices[vertIndex].x;
        verticesWithNormals[vertIndex*6+1] = vertices[vertIndex].y;
        verticesWithNormals[vertIndex*6+2] = vertices[vertIndex].z;
        verticesWithNormals[vertIndex*6+3] = normals[vertIndex].x;
        verticesWithNormals[vertIndex*6+4] = normals[vertIndex].y;
        verticesWithNormals[vertIndex*6+5] = normals[vertIndex].z;
    }
    return verticesWithNormals;
}

void examineCubeWithInterpolation(Triangles &triangles, const GridLayout &layout, float ** grid, int vertex0, float isoValue) {
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
    if (grid[vertices[0]][3] < isoValue) cubeIndex |= 1;
    if (grid[vertices[1]][3] < isoValue) cubeIndex |= 2;
    if (grid[vertices[2]][3] < isoValue) cubeIndex |= 4;
    if (grid[vertices[3]][3] < isoValue) cubeIndex |= 8;
    if (grid[vertices[4]][3] < isoValue) cubeIndex |= 16;
    if (grid[vertices[5]][3] < isoValue) cubeIndex |= 32;
    if (grid[vertices[6]][3] < isoValue) cubeIndex |= 64;
    if (grid[vertices[7]][3] < isoValue) cubeIndex |= 128;

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
            float *endPoint0 = grid[vertices[endPoints[0]]];
            float *endPoint1 = grid[vertices[endPoints[1]]];
            float isoDistance = (isoValue-endPoint0[3])/(endPoint1[3] - endPoint0[3]); 
            glm::vec3 vertex(
                endPoint0[0]+(isoDistance * (endPoint1[0]-endPoint0[0])),
                endPoint0[1]+(isoDistance * (endPoint1[1]-endPoint0[1])),
                endPoint0[2]+(isoDistance * (endPoint1[2]-endPoint0[2]))
            );
            triangles.vertices.push_back(vertex);
            if (endPoint0[3] < isoValue) {
                triangles.activeVertices.insert({endPoint0[0], endPoint0[1], endPoint0[2]});
                triangles.inactiveVertices.insert({endPoint1[0], endPoint1[1], endPoint1[2]});

            } else {
                triangles.inactiveVertices.insert({endPoint0[0], endPoint0[1], endPoint0[2]});
                triangles.activeVertices.insert({endPoint1[0], endPoint1[1], endPoint1[2]});
            }
        }

        glm::vec3 vertex1 = triangles.vertices.at(triangles.vertices.size() - 3);
        glm::vec3 vertex2 = triangles.vertices.at(triangles.vertices.size() - 2);
        glm::vec3 vertex3 = triangles.vertices.at(triangles.vertices.size() - 1);
        glm::vec3 normal = glm::normalize(glm::cross(vertex2 - vertex1, vertex3 - vertex1));
        triangles.normals.push_back(normal);
        triangles.normals.push_back(normal);
        triangles.normals.push_back(normal);
    }
}

Triangles extractTrianglesWithInterpolation(float **grid, const GridLayout &layout, float isoValue) {
    Triangles triangles;

    for (int x = 0; x < layout.x - 1; ++x) {
    for (int y = 0; y < layout.y - 1; ++y) {
    for (int z = 0; z < layout.z - 1; ++z) {
        examineCubeWithInterpolation(triangles, layout, grid, (x*layout.z*layout.y) + (y*layout.z) + z, isoValue);
    }}}

    std::cout << "Generated Surface:" << std::endl;
    std::cout << "Active Vertices: " << triangles.activeVertices.size() << std::endl;
    std::cout << "Inactive Vertices: " << triangles.inactiveVertices.size() << std::endl;

    return triangles;
}