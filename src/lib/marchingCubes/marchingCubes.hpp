#pragma once
#include <stdlib.h>
#include <glm.hpp>
#include <vector>
#include <map>
#include <set>

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

struct GridLayout {
	int x, y, z, total;
	GridLayout(int x, int y, int z): x(x), y(y), z(z) {
		total = x*y*z;
	}
};

struct Surface {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;

	std::set<glm::vec3, Vec3Comparator> activeVertices;
	std::set<glm::vec3, Vec3Comparator> inactiveVertices;

	std::vector<float> getVertices();
};

void examineCube(Surface &triangles, const GridLayout &layout, const  std::vector<std::vector<float>> &grid, int vertex0, float isoValue);
Surface extractSurface(const std::vector<std::vector<float>> &grid, const GridLayout &layout, float isoValue);
