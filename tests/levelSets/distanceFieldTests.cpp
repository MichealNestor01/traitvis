#include "distanceField.hpp"
#include "multiField.hpp"
#include "scalarField.hpp"

#include <gtest/gtest.h>
#include <glm.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>


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
            .indexFunction = [](int x, int y, int z, int xVals, int yVals, int zVals) {
                return z + zVals * (y + yVals * x); // comlumn majour, width_hieght_depth dimension order
            }
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

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    ScalarField field = generateDistanceField(points, testMultiField);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

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

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    ScalarField field = generateDistanceField(points, testMultiField);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

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

    // Redirect std::cout
    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());

    ScalarField field = generateDistanceField(points, testMultiField);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    for (int i = 0; i < static_cast<int>(field.values.size()); i++) {
        const glm::ivec3 c = field.layout.coords(i);
        EXPECT_EQ(field.values[i], expectedDistanceField[i]) << " coordinate (" << c.x << ", " << c.y << ", " << c.z << ") has distance " << field.values[i] << " expected: " << expectedDistanceField[i];
    }
}

TEST_F(DistanceFieldTests, NaNVoxelKeepsMaxDistanceAndDoesNotAffectNeighbours) {
    testMultiField.attributeDomain[0].values[0] = std::numeric_limits<float>::quiet_NaN();
    std::vector<TraitPoint> points = {pointOn(0, 1.f)};

    std::streambuf* originalCoutBuffer = std::cout.rdbuf();
    std::ostringstream redirectedCout;
    std::cout.rdbuf(redirectedCout.rdbuf());
    ScalarField field = generateDistanceField(points, testMultiField);
    std::cout.rdbuf(originalCoutBuffer);

    // (0,0,0) holds the NaN. NaN loses the comparison, so the voxel stays at the initial max.
    EXPECT_EQ(field.values[0], std::numeric_limits<float>::max());
    // (0,0,1) has attribute value 1, so its distance to the trait value 1 is unchanged.
    EXPECT_FLOAT_EQ(field.values[1], 0.f);
}
