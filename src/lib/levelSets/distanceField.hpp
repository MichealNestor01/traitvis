#pragma once
#include <vector>
#include "../multiField/multiField.hpp"
#include "scalarField.hpp"

ScalarField generateDistanceField(const std::vector<TraitPoint> & vertices, const MultiField & multifield);
