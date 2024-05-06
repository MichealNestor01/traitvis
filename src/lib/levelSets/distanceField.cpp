#include <glm.hpp>
#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "distanceField.hpp"

std::vector<std::vector<float>> generateDistanceField(const std::vector<TraitPoint> & vertices, const MultiField & multifield) {
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    //float ** distanceField = (float**)malloc(sizeof(float*) * multifield.xVals * multifield.yVals * multifield.zVals);
    std::vector<std::vector<float>> distanceField(multifield.xVals * multifield.yVals * multifield.zVals, std::vector<float>(4));
    int index = 0;
    for (int x = 0; x < multifield.xVals; ++x) {
    for (int y = 0; y < multifield.yVals; ++y) {
    for (int z = 0; z < multifield.zVals; ++z) {
        // set the current points coordinates
        distanceField[index][0] = static_cast<float>(x);
        distanceField[index][1] = static_cast<float>(y);
        distanceField[index][2] = static_cast<float>(z);
        int indexInDataset = multifield.getIndexInDataset(x, y, z);
        // calculate the distance form the current point ot the closest attribute vertex
        float distance = std::numeric_limits<float>::max();
        for (const TraitPoint & vertex : vertices) {
            // calcuate distance from current vertex
            float euclideanDistanceSum = 0;
            for (const TraitPointComponent & value : vertex.values) {
                float component = value.attribute->values[indexInDataset] - value.value;
                euclideanDistanceSum += component * component;
            }
            float euclideanDistance = sqrt(euclideanDistanceSum);
            if (euclideanDistance < distance) distance = euclideanDistance;
        }
        // finally set the current points distance
        distanceField[index][3] = distance;
        index++;
    }}}

    // End timing and calculate duration
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cout << "Distance Field time taken: " << elapsed.count() << " seconds\n";

    return distanceField;
}