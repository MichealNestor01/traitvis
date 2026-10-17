#include "histogram.hpp"

#include <cmath>

namespace {

// Which bin `value` belongs in. `bins` is at least 1.
// Scaling stays in float: 0.9f is slightly under 0.9, and promoting that
// product to double drops it into the previous bin.
[[nodiscard]] int binIndex(float value, int bins, FloatRange range) {
    const float span = range.upper - range.lower;
    if (!(span > 0.f)) return value < range.lower ? 0 : bins - 1;
    if (value >= range.upper) return bins - 1;
    if (value <= range.lower) return 0;

    const float scaled = (value - range.lower) / span * static_cast<float>(bins);
    if (!(scaled > 0.f)) return 0;
    if (!(scaled < static_cast<float>(bins))) return bins - 1;
    return static_cast<int>(scaled);
}

}

std::vector<int> histogram(std::span<const float> values, int bins, FloatRange range) {
    if (bins <= 0) return {};

    std::vector<int> counts(static_cast<std::size_t>(bins), 0);
    for (const float value : values) {
        if (std::isnan(value)) continue;
        ++counts[static_cast<std::size_t>(binIndex(value, bins, range))];
    }
    return counts;
}
