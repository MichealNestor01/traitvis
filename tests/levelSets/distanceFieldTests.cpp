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
        sqrt(2), 0,         // y = 0
        sqrt(2), sqrt(2),   // y = 2
        // x layer 2
        sqrt(2), sqrt(2),   // y = 0
        1,       sqrt(2),   // y = 1
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
        sqrt(3), 0,         // y = 0
        sqrt(3), sqrt(3),   // y = 2
        // x layer 2
        sqrt(3), sqrt(3),   // y = 0
        1,       sqrt(2),   // y = 1
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
