#include "featureLevelSet.hpp"
#include <iostream>

FeatureLevelSet::FeatureLevelSet(std::vector<AttributeVertex> vertices, MultiField & dataset, float euclidianDistance, glm::vec3 colour, std::string id) {
    this->colour = colour;
    this->id = id;
    this->vertices = vertices;
    // first normalise the values of the vertices chosen
    for (AttributeVertex & vertex : this->vertices) vertex.normalise();
    // create the distance field
    float ** distanceField = generateDistanceField(this->vertices, dataset);
    // extract the surface given the normalised eucliean distance
    surface = extractTrianglesWithInterpolation(distanceField, {dataset.xVals, dataset.yVals, dataset.zVals}, euclidianDistance);
    surfaceVertices = surface.getVertices();
    // free distanceField
    for (int i = (dataset.xVals * dataset.yVals * dataset.zVals)-1; i >= 0; --i) free(distanceField[i]);
    free(distanceField);
}   

float ** FeatureLevelSet::generateDistanceField(const std::vector<AttributeVertex> & vertices, const MultiField & multifield) {
    float ** distanceField = (float**)malloc(sizeof(float*) * multifield.xVals * multifield.yVals * multifield.zVals);
    int index = 0;

    bool bogo = false;
    for (int x = 0; x < multifield.xVals; ++x) {
    for (int z = 0; z < multifield.zVals; ++z) {
    for (int y = 0; y < multifield.yVals; ++y) {
        distanceField[index] = (float*)malloc(sizeof(float) * 4);
        distanceField[index][0] = static_cast<float>(x);
        distanceField[index][1] = static_cast<float>(y);
        distanceField[index][2] = static_cast<float>(z);
        distanceField[index][3] = -1.f;
        int indexInDataset2 = multifield.getIndexInDataset(x, y, z);
        // calculate the distance form the current point ot the closest attribute vertex
        float distance = std::numeric_limits<float>::max();
        for (const AttributeVertex & vertex : vertices) {
            // calcuate distance from current vertex
            float euclidianDistanceSum = 0;
            for (const AttributeVertexValue & value : vertex.values) {
                // if (false and value.attribute->values[indexInDataset] >= value.attribute->bounds.lower and value.attribute->values[indexInDataset] <= value.attribute->bounds.upper)  {
                //     std::cout << "Value: " << value.attribute->values[indexInDataset] << " is within bounds: " << value.attribute->bounds.lower << " and " << value.attribute->bounds.upper << std::endl;
                //     bogo = true;
                // }
                float component = value.attribute->values[indexInDataset] - value.value;
                euclidianDistanceSum += component * component;
            }
            float euclidianDistance = sqrt(euclidianDistanceSum);
            if (euclidianDistance < distance) distance = euclidianDistance;
        }
        distanceField[index][3] = distance;

        // if (bogo and false) {
        //     std::cout << "Point: " << x << ", " << y << ", " << z << std::endl;
        //     std::cout << "Distance: " << distance << std::endl;
        //     exit(0);
        // }

        index++;
    }}}
    return distanceField;
}