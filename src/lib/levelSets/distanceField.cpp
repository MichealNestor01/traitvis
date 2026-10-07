#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include "distanceField.hpp"

ScalarField generateDistanceField(const std::vector<TraitPoint> & vertices, const MultiField & multifield) {
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    ScalarField field{
        .layout = {multifield.xVals, multifield.yVals, multifield.zVals},
    };
    field.values.assign(field.layout.total(), 0.f);
    for (int x = 0; x < multifield.xVals; ++x) {
    for (int y = 0; y < multifield.yVals; ++y) {
    for (int z = 0; z < multifield.zVals; ++z) {
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
            float euclideanDistance = std::sqrt(euclideanDistanceSum);
            if (euclideanDistance < distance) distance = euclideanDistance;
        }
        field.values[field.layout.index(x, y, z)] = distance;
    }}}

    // End timing and calculate duration
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    std::cout << "Distance Field time taken: " << elapsed.count() << " seconds\n";

    return field;
}