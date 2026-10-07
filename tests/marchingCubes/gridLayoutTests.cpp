#include "gridLayout.hpp"

#include <gtest/gtest.h>

TEST(GridLayoutTests, IndexAndCoordsRoundTrip) {
    constexpr GridLayout layout{2, 3, 4};
    for (int i = 0; i < layout.total(); ++i) {
        const glm::ivec3 c = layout.coords(i);
        EXPECT_EQ(layout.index(c.x, c.y, c.z), i);
    }
    static_assert(GridLayout{3, 3, 3}.index(1, 1, 1) == 13);
}
