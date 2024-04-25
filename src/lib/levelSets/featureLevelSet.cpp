#include "featureLevelSet.hpp"
#include <iostream>

FeatureLevelSet::FeatureLevelSet(std::vector<AttributeVertex> vertices, MultiField & dataset, float euclidianDistance, glm::vec3 colour, std::string id) {
    this->colour = colour;
    this->id = id;
    this->vertices = vertices;
    // first normalise the values of the vertices chosen
    for (AttributeVertex & vertex : this->vertices) vertex.normalise();
    // create the distance field
    std::vector<std::vector<float>> distanceField = generateDistanceField(this->vertices, dataset);
    // extract the surface given the normalised eucliean distance
    surface = extractSurface(distanceField, {dataset.xVals, dataset.yVals, dataset.zVals}, euclidianDistance);
    surfaceVertices = surface.getVertices();
}   

std::vector<std::vector<float>> FeatureLevelSet::generateDistanceField(const std::vector<AttributeVertex> & vertices, const MultiField & multifield) {
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