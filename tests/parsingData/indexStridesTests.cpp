#include "datasetConfig.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>
#include <vector>

struct StrideCase {
    const char* name;
    IndexScheme scheme;
    DimensionOrder order;
    int indexOf102;
};

class IndexStridesTests : public testing::TestWithParam<StrideCase> {};

TEST_P(IndexStridesTests, MatchesOriginalFormulaAndIsABijection) {
    const auto [name, scheme, order, expected] = GetParam();
    const IndexStrides s = makeStrides(scheme, order, 2, 3, 4);
    EXPECT_EQ(s.index(1, 0, 2), expected) << name;

    std::vector<int> seen;
    for (int x = 0; x < 2; ++x)
        for (int y = 0; y < 3; ++y)
            for (int z = 0; z < 4; ++z)
                seen.push_back(s.index(x, y, z));
    std::ranges::sort(seen);
    EXPECT_TRUE(std::ranges::equal(seen, std::views::iota(0, 24)));
}

INSTANTIATE_TEST_SUITE_P(
    Schemes,
    IndexStridesTests,
    testing::Values(
        StrideCase{"RowMajorWidthHeightDepth",  ROWMAJOR,    WIDTH_HEIGHT_DEPTH, 13},
        StrideCase{"RowMajorWidthDepthHeight",  ROWMAJOR,    WIDTH_DEPTH_HEIGHT,  5},
        StrideCase{"ColumnMajorWidthHeightDepth", COLUMNMAJOR, WIDTH_HEIGHT_DEPTH, 14},
        StrideCase{"ColumnMajorWidthDepthHeight", COLUMNMAJOR, WIDTH_DEPTH_HEIGHT, 18}
    ),
    [](const testing::TestParamInfo<StrideCase>& info) {
        return info.param.name;
    }
);
