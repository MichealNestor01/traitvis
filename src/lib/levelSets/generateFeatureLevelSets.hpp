#pragma once
#include "../multiField/multiField.hpp"
#include <vector>

float ** generateDistanceField(const std::vector<AttributeVertex> & vertices, const MultiField & mulitifield);
//float ** generateFeatureLevelSet(const std::vector<AttributeVertex> & vertices, const MultiField & mulitifield, float levelSetDistance);