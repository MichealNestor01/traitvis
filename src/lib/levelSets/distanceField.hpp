#pragma once
#include <vector>
#include "../multiField/multiField.hpp"

std::vector<std::vector<float>> generateDistanceField(const std::vector<TraitPoint> & vertices, const MultiField & mulitifield);