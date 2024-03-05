#pragma once
#include <stdlib.h>
#include <glm.hpp>
#include <vector>

typedef struct GridLayout {
	int x, y, z, total;
	GridLayout(int x, int y, int z): x(x), y(y), z(z) {
		total = x*y*z;
	}
} GridLayout;

typedef struct Triangles {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;

	void computeNormals();
	std::vector<float> getVertices();
} Triangles;

Triangles extractTriangles(int **grid, const GridLayout &layout);
void examineCube(Triangles &triangles, const GridLayout &layout, int **grid, int vertex0);

Triangles extractTrianglesWithInterpolation(float **grid, const GridLayout &layout, float isoValue);
void examineCubeWithInterpolation(Triangles &triangles, const GridLayout &layout, float ** grid, int vertex0, float isoValue);

