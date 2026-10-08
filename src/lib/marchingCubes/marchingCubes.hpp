#pragma once
#include <stdlib.h>
#include <vector>
#include <map>
#include <set>
#include "../levelSets/scalarField.hpp"

struct Vec3Comparator {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        // First compare x, then y, then z
        if (a.x < b.x) return true;
        if (a.x > b.x) return false;
        if (a.y < b.y) return true;
        if (a.y > b.y) return false;
        return a.z < b.z;
    }
};

struct Surface {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;

	std::set<glm::vec3, Vec3Comparator> activeVertices;
	std::set<glm::vec3, Vec3Comparator> inactiveVertices;

	std::vector<float> interleaved() const;
};

void examineCube(Surface &triangles, const ScalarField &field, int vertex0, float isoValue);
Surface extractSurface(const ScalarField &field, float isoValue);
