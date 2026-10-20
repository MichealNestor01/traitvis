#pragma once

#include "scalarField.hpp"

#include <glm.hpp>
#include <vector>

inline ScalarField radialField(int n) {
    ScalarField f{{n, n, n}, std::vector<float>(static_cast<std::size_t>(n * n * n))};
    const glm::vec3 centre((n - 1) / 2.f);
    for (int i = 0; i < f.layout.total(); ++i)
        f.values[i] = glm::distance(glm::vec3(f.layout.coords(i)), centre);
    return f;
}
