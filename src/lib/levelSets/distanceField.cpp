#include <glm.hpp>
#include <stdlib.h>
#include "distanceField.hpp"

std::vector<std::vector<float>> generateDistanceField(const std::vector<AttributeVertex> & vertices, const MultiField & multifield) {
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
        for (const AttributeVertex & vertex : vertices) {
            // calcuate distance from current vertex
            float euclidianDistanceSum = 0;
            for (const AttributeVertexValue & value : vertex.values) {
                float component = value.attribute->values[indexInDataset] - value.value;
                euclidianDistanceSum += component * component;
            }
            float euclidianDistance = sqrt(euclidianDistanceSum);
            if (euclidianDistance < distance) distance = euclidianDistance;
        }
        // finally set the current points distance
        distanceField[index][3] = distance;
        index++;
    }}}
    return distanceField;
}