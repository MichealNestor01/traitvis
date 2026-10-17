#include "histogram.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <vector>

TEST(HistogramTests, SamplesFallIntoTheSpecifiedBins) {
    const std::vector<float> values{0.0f, 0.1f, 0.1f, 0.5f, 0.9f, 0.99f};
    EXPECT_EQ(histogram(values, 10, {0.f, 1.f}),
              (std::vector<int>{1, 2, 0, 0, 0, 1, 0, 0, 0, 2}));
}

TEST(HistogramTests, NaNIsSkipped) {
    const std::vector<float> values{std::numeric_limits<float>::quiet_NaN()};
    EXPECT_EQ(histogram(values, 10, {0.f, 1.f}), std::vector<int>(10, 0));
}

TEST(HistogramTests, UpperBoundLandsInTheLastBin) {
    EXPECT_EQ(histogram(std::vector<float>{1.f}, 10, {0.f, 1.f}),
              (std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0, 0, 1}));
}

TEST(HistogramTests, ValuesOutsideTheRangeClampToTheEdgeBins) {
    const std::vector<float> values{-5.f, 7.f};
    EXPECT_EQ(histogram(values, 10, {0.f, 1.f}),
              (std::vector<int>{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}));
}
