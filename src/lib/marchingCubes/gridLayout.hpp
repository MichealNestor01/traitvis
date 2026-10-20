#pragma once
#include <glm.hpp>

struct GridLayout {
    int x, y, z;
    [[nodiscard]] constexpr int total() const noexcept { return x * y * z; }
    // Storage order: x outermost, z innermost — the order examineCube already assumes.
    [[nodiscard]] constexpr int index(int ix, int iy, int iz) const noexcept { return (ix * y + iy) * z + iz; }
    [[nodiscard]] constexpr glm::ivec3 coords(int i) const noexcept { return { i / (y * z), (i / z) % y, i % z }; }
};
