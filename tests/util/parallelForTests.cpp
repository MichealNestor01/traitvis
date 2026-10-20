#include "parallelFor.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <vector>

TEST(ParallelForTests, VisitsEveryIndexExactlyOnce) {
    constexpr int n = 1000;   // not a multiple of any plausible thread count
    std::vector<std::atomic<int>> visits(n);
    parallelFor(0, n, [&](int i) { ++visits[i]; });
    EXPECT_TRUE(std::ranges::all_of(visits, [](const auto& v) { return v.load() == 1; }));
}

TEST(ParallelForTests, EmptyRangeCallsNothing) {
    std::atomic<int> calls{0};
    parallelFor(5, 5, [&](int) { ++calls; });
    EXPECT_EQ(calls.load(), 0);
}
