#include "distanceField.hpp"
#include "multiField.hpp"
#include "scalarField.hpp"

#include <gtest/gtest.h>
#include <glm.hpp>
#include <cmath>
#include <limits>
#include <utility>


// Using a test class here to ensure that the same MultiField object is used for all tests
class DistanceFieldTests : public ::testing::Test {
protected:
    MultiField testMultiField;

    void SetUp() override {
        testMultiField = {
            .name = "testMultiField",
            .xVals = 2,
            .yVals = 2,
            .zVals = 2,
            .attributeDomain = { // std::vector<Attribute>
                {
                    .name = "testAttribute1",
                    .bounds = {0, 1},
                    .values = { // std::vector<float>
                        // x layer 1
                        0, 1, // y = 0
                        0, 0, // y = 1
                        // x layer 2
                        0, 0, // y = 0
                        0, 0, // y = 1
                    }
                },
                {
                    .name = "testAttribute2",
                    .bounds = {0, 1},
                    .values = { // std::vector<float>
                        // x layer 1
                        0, 1, // y = 0
                        0, 0, // y = 1
                        // x layer 2
                        0, 0, // y = 0
                        1, 0, // y = 1
                    }
                },
                {
                    .name = "testAttribute3",
                    .bounds = {0, 1},
                    .values = { // std::vector<float>
                        // x layer 1
                        0, 1, // y = 0
                        0, 0, // y = 1
                        // x layer 2
                        0, 0, // y = 0
                        1, 1, // y = 1
                    }
                }
            },
            .strides = {4, 2, 1} // column major, width-height-depth, on a 2x2x2 grid
        };
    }

    // Fixture method: it has to see testMultiField, so it cannot be a free function.
    TraitPoint pointOn(int attribute, float value) {
        return {.values = {{.attribute = &testMultiField.attributeDomain[attribute], .value = value}}};
    }
};


TEST_F(DistanceFieldTests, SingleDimensionAttributeSpace) {
    std::vector<TraitPoint> points = {pointOn(0, 1.f)};

    // simplified distance field as the coordinates are not needed for this test
    std::vector<float> expectedDistanceField = {
        // x layer 1
        1, 0, // y = 0
        1, 1, // y = 2
        // x layer 2
        1, 1, // y = 0
        1, 1, // y = 1
    };

    ScalarField field = generateDistanceField(points, testMultiField);

    for (int i = 0; i < static_cast<int>(field.values.size()); i++) {
        const glm::ivec3 c = field.layout.coords(i);
        EXPECT_EQ(field.values[i], expectedDistanceField[i]) << " coordinate (" << c.x << ", " << c.y << ", " << c.z << ") has distance " << field.values[i] << " expected: " << expectedDistanceField[i];
    }
}

TEST_F(DistanceFieldTests, TwoDimensionalAttributeSpace) {
    TraitPoint point1 = pointOn(0, 1.f);
    point1.values.push_back(pointOn(1, 1.f).values[0]);

    std::vector<TraitPoint> points = {point1};

    // simplified distance field as the coordinates are not needed for this test
    std::vector<float> expectedDistanceField = {
        // x layer 1
        std::sqrt(2.f), 0,               // y = 0
        std::sqrt(2.f), std::sqrt(2.f), // y = 2
        // x layer 2
        std::sqrt(2.f), std::sqrt(2.f), // y = 0
        1,              std::sqrt(2.f), // y = 1
    };

    ScalarField field = generateDistanceField(points, testMultiField);

    for (int i = 0; i < static_cast<int>(field.values.size()); i++) {
        const glm::ivec3 c = field.layout.coords(i);
        EXPECT_EQ(field.values[i], expectedDistanceField[i]) << " coordinate (" << c.x << ", " << c.y << ", " << c.z << ") has distance " << field.values[i] << " expected: " << expectedDistanceField[i];
    }
}

TEST_F(DistanceFieldTests, ThreeDimensionalAttributeSpace) {
    TraitPoint point1 = pointOn(0, 1.f);
    point1.values.push_back(pointOn(1, 1.f).values[0]);
    point1.values.push_back(pointOn(2, 1.f).values[0]);

    std::vector<TraitPoint> points = {point1};

    // simplified distance field as the coordinates are not needed for this test
    std::vector<float> expectedDistanceField = {
        // x layer 1
        std::sqrt(3.f), 0,               // y = 0
        std::sqrt(3.f), std::sqrt(3.f), // y = 2
        // x layer 2
        std::sqrt(3.f), std::sqrt(3.f), // y = 0
        1,              std::sqrt(2.f), // y = 1
    };

    ScalarField field = generateDistanceField(points, testMultiField);

    for (int i = 0; i < static_cast<int>(field.values.size()); i++) {
        const glm::ivec3 c = field.layout.coords(i);
        EXPECT_EQ(field.values[i], expectedDistanceField[i]) << " coordinate (" << c.x << ", " << c.y << ", " << c.z << ") has distance " << field.values[i] << " expected: " << expectedDistanceField[i];
    }
}

TEST_F(DistanceFieldTests, NaNVoxelKeepsMaxDistanceAndDoesNotAffectNeighbours) {
    testMultiField.attributeDomain[0].values[0] = std::numeric_limits<float>::quiet_NaN();
    std::vector<TraitPoint> points = {pointOn(0, 1.f)};

    ScalarField field = generateDistanceField(points, testMultiField);

    // (0,0,0) holds the NaN. NaN loses the comparison, so the voxel stays at the initial max.
    EXPECT_EQ(field.values[0], std::numeric_limits<float>::max());
    // (0,0,1) has attribute value 1, so its distance to the trait value 1 is unchanged.
    EXPECT_FLOAT_EQ(field.values[1], 0.f);
}

namespace {

// The desktop compute shader, written out as CPU so the packing and the formula can
// be checked without an OpenGL context. Squares are summed in full, then compared.
float shaderDistance(const MultiField& field, const DistanceFieldDispatch& packed, int voxel) {
    const glm::ivec3 coordinate = packed.layout.coords(voxel);
    const int sampleIndex = packed.strides.index(coordinate.x, coordinate.y, coordinate.z);
    float minDistSq = std::numeric_limits<float>::infinity();
    for (const DistanceFieldPoint& point : packed.points) {
        float sum = 0.f;
        bool rejected = false;
        for (int componentIndex = 0; componentIndex < point.count; ++componentIndex) {
            if (sampleIndex < 0 || sampleIndex >= packed.sampleCount) {
                rejected = true;
                break;
            }
            const DistanceFieldComponent& component = packed.components[point.begin + componentIndex];
            const float sample = field.attributeDomain[component.attribute].values[sampleIndex];
            const float difference = sample - component.target;
            const float squared = difference * difference;
            if (!std::isfinite(squared)) {
                rejected = true;
                break;
            }
            sum += squared;
        }
        if (!rejected && sum < minDistSq) minDistSq = sum;
    }
    if (!std::isfinite(minDistSq)) return std::numeric_limits<float>::max();
    return std::sqrt(minDistSq);
}

void expectShaderMatchesCpu(const MultiField& field, const std::vector<TraitPoint>& points) {
    const std::optional<DistanceFieldDispatch> packed = packDistanceField(points, field);
    ASSERT_TRUE(packed);
    const ScalarField cpu = generateDistanceField(points, field);
    ASSERT_EQ(static_cast<int>(cpu.values.size()), packed->layout.total());
    for (int i = 0; i < packed->layout.total(); ++i)
        EXPECT_EQ(cpu.values[i], shaderDistance(field, *packed, i)) << "voxel " << i;
}

}

TEST_F(DistanceFieldTests, PackGroupsComponentsInTraitPointOrder) {
    TraitPoint both = pointOn(2, 0.25f);
    both.values.push_back(pointOn(0, 0.5f).values[0]);
    const std::vector<TraitPoint> points = {both, pointOn(1, 0.75f)};

    const std::optional<DistanceFieldDispatch> packed = packDistanceField(points, testMultiField);
    ASSERT_TRUE(packed);
    EXPECT_EQ(packed->sampleCount, 8);
    EXPECT_EQ(packed->strides.x, 4);
    ASSERT_EQ(packed->points.size(), 2u);
    EXPECT_EQ(packed->points[0].begin, 0);
    EXPECT_EQ(packed->points[0].count, 2);
    EXPECT_EQ(packed->points[1].begin, 2);
    EXPECT_EQ(packed->points[1].count, 1);
    ASSERT_EQ(packed->components.size(), 3u);
    EXPECT_EQ(packed->components[0].attribute, 2);
    EXPECT_FLOAT_EQ(packed->components[0].target, 0.25f);
    EXPECT_EQ(packed->components[1].attribute, 0);
    EXPECT_FLOAT_EQ(packed->components[1].target, 0.5f);
    EXPECT_EQ(packed->components[2].attribute, 1);
    EXPECT_FLOAT_EQ(packed->components[2].target, 0.75f);
}

TEST_F(DistanceFieldTests, PackRejectsAnAttributeFromOutsideTheDataset) {
    Attribute outsider;
    outsider.values = {0.f};
    const TraitPoint point{.values = {{&outsider, 1.f}}};
    EXPECT_FALSE(packDistanceField(std::vector<TraitPoint>{point}, testMultiField));
}

TEST_F(DistanceFieldTests, PackRejectsAttributesOfDifferentLengths) {
    testMultiField.attributeDomain[1].values.pop_back();
    EXPECT_FALSE(packDistanceField({}, testMultiField));
}

TEST_F(DistanceFieldTests, ShaderFormulaMatchesTheCpuField) {
    {
        SCOPED_TRACE("one trait");
        ASSERT_NO_FATAL_FAILURE(expectShaderMatchesCpu(testMultiField, {pointOn(0, 1.f)}));
    }
    {
        SCOPED_TRACE("three components");
        TraitPoint point = pointOn(0, 1.f);
        point.values.push_back(pointOn(1, 1.f).values[0]);
        point.values.push_back(pointOn(2, 1.f).values[0]);
        ASSERT_NO_FATAL_FAILURE(expectShaderMatchesCpu(testMultiField, {point}));
    }
    {
        SCOPED_TRACE("nan trait loses to a finite trait");
        testMultiField.attributeDomain[0].values[0] = std::numeric_limits<float>::quiet_NaN();
        ASSERT_NO_FATAL_FAILURE(expectShaderMatchesCpu(testMultiField, {pointOn(0, 1.f), pointOn(1, 0.f)}));
    }
}

TEST(DistanceFieldDispatchTests, ShaderFormulaUsesStridesForSamplesAndLayoutForOutput) {
    MultiField field;
    field.xVals = 2;
    field.yVals = 2;
    field.zVals = 2;
    field.strides = {1, 2, 4};
    Attribute attribute;
    attribute.bounds = {0.f, 1.f};
    attribute.values.assign(8, 0.f);
    attribute.values[field.strides.index(1, 0, 0)] = 1.f;
    field.attributeDomain.push_back(std::move(attribute));
    const std::vector<TraitPoint> points = {{.values = {{&field.attributeDomain[0], 1.f}}}};

    ASSERT_NO_FATAL_FAILURE(expectShaderMatchesCpu(field, points));
    const ScalarField cpu = generateDistanceField(points, field);
    EXPECT_NE(cpu.layout.index(1, 0, 0), field.strides.index(1, 0, 0));
    EXPECT_EQ(cpu.values[cpu.layout.index(1, 0, 0)], 0.f);
}

TEST_F(DistanceFieldTests, SteppingSlabBySlabMatchesOneShotAndReportsProgress) {
    std::vector<TraitPoint> points = {pointOn(0, 1.f)};

    const ScalarField expected = generateDistanceField(points, testMultiField);

    DistanceFieldJob job(points, testMultiField);      // xVals == 2 → two slabs
    EXPECT_FLOAT_EQ(job.progress(), 0.f);
    EXPECT_FALSE(job.step(1));
    EXPECT_FLOAT_EQ(job.progress(), 0.5f);
    EXPECT_TRUE(job.step(1));
    EXPECT_TRUE(job.step(1));                          // no-op once done
    EXPECT_FLOAT_EQ(job.progress(), 1.f);
    EXPECT_EQ(std::move(job).take().values, expected.values);
}
