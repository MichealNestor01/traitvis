#pragma once
#include <stdlib.h>
#include "marchingCubes.hpp"

int ** createGrid(GridLayout layout, std::vector<glm::vec3> activeVertices) {
	int ** grid = (int**)malloc(sizeof(int*) * layout.x * layout.y * layout.z);
	int index = 0;
    for (int x = 0; x < layout.x; ++x) {
    for (int y = 0; y < layout.y; ++y) {
    for (int z = 0; z < layout.z; ++z) {
		grid[index] = (int*)malloc(sizeof(int) * 4);
		grid[index][0] = x;
		grid[index][1] = y;
		grid[index][2] = z;
		grid[index][3] = 0;
		for (glm::vec3 vertex : activeVertices) 
			if (glm::vec3(x, y, z) == vertex) grid[index][3] = 1;
		index++;
    }}}
	return grid;
}
