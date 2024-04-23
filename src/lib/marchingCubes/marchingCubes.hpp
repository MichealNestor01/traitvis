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

typedef struct GridLayout {
	int x, y, z, total;
	GridLayout(int x, int y, int z): x(x), y(y), z(z) {
		total = x*y*z;
	}
} GridLayout;

typedef struct Triangles {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;

	std::set<glm::vec3, Vec3Comparator> activeVertices;
	std::set<glm::vec3, Vec3Comparator> inactiveVertices;

	void computeNormals();
	std::vector<float> getVertices();
} Triangles;

Triangles extractTrianglesWithInterpolation(float **grid, const GridLayout &layout, float isoValue);
void examineCubeWithInterpolation(Triangles &triangles, const GridLayout &layout, float ** grid, int vertex0, float isoValue, std::map<float, int> &zDistribution);

