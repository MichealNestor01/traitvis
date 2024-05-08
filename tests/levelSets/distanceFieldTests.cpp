#include "distanceField.hpp"
#include "multiField.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <cmath>


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
};


TEST_F(DistanceFieldTests, SingleDimensionAttributeSpace) {
    TraitPoint point1 = {
        .values = { // std::vector<TraitPointComponent>
            {
                .attribute = &testMultiField.attributeDomain[0],
                .value = 1
            }
        }
    };
    
    std::vector<TraitPoint> points = {point1};

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

    std::vector<std::vector<float>> distanceField = generateDistanceField(points, testMultiField);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    // validate that the distance field matches the expected distance field 
    for (int i = 0; i < distanceField.size(); i++) 
        EXPECT_EQ(distanceField[i][3], expectedDistanceField[i]) << " coordinate (" << distanceField[i][0] << ", " << distanceField[i][1] << "," << distanceField[i][2] << ") has distance " << distanceField[i][3] << " expected: " << expectedDistanceField[i];
}    

TEST_F(DistanceFieldTests, TwoDimensionalAttributeSpace) {
    TraitPoint point1 = {
        .values = { // std::vector<TraitPointComponent>
            {
                .attribute = &testMultiField.attributeDomain[0],
                .value = 1
            },
            {
                .attribute = &testMultiField.attributeDomain[1],
                .value = 1
            }
        }
    };
    
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

    std::vector<std::vector<float>> distanceField = generateDistanceField(points, testMultiField);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    // validate that the distance field matches the expected distance field 
    for (int i = 0; i < distanceField.size(); i++) 
        EXPECT_EQ(distanceField[i][3], expectedDistanceField[i]) << " coordinate (" << distanceField[i][0] << ", " << distanceField[i][1] << "," << distanceField[i][2] << ") has distance " << distanceField[i][3] << " expected: " << expectedDistanceField[i];
}    

TEST_F(DistanceFieldTests, ThreeDimensionalAttributeSpace) {
    TraitPoint point1 = {
        .values = { // std::vector<TraitPointComponent>
            {
                .attribute = &testMultiField.attributeDomain[0],
                .value = 1
            },
            {
                .attribute = &testMultiField.attributeDomain[1],
                .value = 1
            },
            {
                .attribute = &testMultiField.attributeDomain[2],
                .value = 1
            }
        }
    };
    
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

    std::vector<std::vector<float>> distanceField = generateDistanceField(points, testMultiField);

    // Restore std::cout to its original buffer
    std::cout.rdbuf(originalCoutBuffer);

    // validate that the distance field matches the expected distance field 
    for (int i = 0; i < distanceField.size(); i++) 
        EXPECT_EQ(distanceField[i][3], expectedDistanceField[i]) << " coordinate (" << distanceField[i][0] << ", " << distanceField[i][1] << ", " << distanceField[i][2] << ") has distance " << distanceField[i][3] << " expected: " << expectedDistanceField[i];
}    
    
    