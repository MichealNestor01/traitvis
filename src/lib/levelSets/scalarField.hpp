#pragma once
#include <vector>
#include "../marchingCubes/gridLayout.hpp"

struct ScalarField {
    GridLayout layout;
    std::vector<float> values;   // layout.total() entries
};
