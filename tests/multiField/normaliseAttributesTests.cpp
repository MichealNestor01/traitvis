#include "multiField.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

TEST(NormaliseAttributesTests, NaNPassesThroughWhileFiniteValuesAreScaled) {
    MultiField field;
    field.attributeDomain.push_back(Attribute{
        .name = "attr",
        .bounds = {0.f, 2.f},
        .values = {1.f, std::numeric_limits<float>::quiet_NaN()},
    });

    field.normaliseAttributes();

    EXPECT_FLOAT_EQ(field.attributeDomain[0].values[0], 0.5f);
    EXPECT_TRUE(std::isnan(field.attributeDomain[0].values[1]));
}
