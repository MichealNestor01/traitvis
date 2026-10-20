#pragma once

#include "multiField.hpp"

#include <span>
#include <vector>

// Counts of how `values` fall across `bins` equal slices of `range`.
// NaN is skipped. Values outside `range` clamp into the end bins.
// A value equal to `range.upper` lands in the last bin.
[[nodiscard]] std::vector<int> histogram(std::span<const float> values, int bins, FloatRange range);
