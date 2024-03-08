#include "featureLevelSet.hpp"

FeatureLevelSet::FeatureLevelSet(std::vector<AttributeVertex> vertices, MultiField & dataset, float euclidianDistance, glm::vec3 colour, std::string id) {
    this->vertices = vertices;
    this->colour = colour;
    this->id = id;
    float ** distanceField = generateDistanceField(vertices, dataset);
    surface = extractTrianglesWithInterpolation(distanceField, {dataset.xVals, dataset.yVals, dataset.zVals}, euclidianDistance);
    surfaceVertices = surface.getVertices();
    // free distanceField
    for (int i = (dataset.xVals * dataset.yVals * dataset.zVals)-1; i >= 0; --i) free(distanceField[i]);
    free(distanceField);
}   

float ** FeatureLevelSet::generateDistanceField(const std::vector<AttributeVertex> & vertices, const MultiField & mulitifield) {
    float ** distanceField = (float**)malloc(sizeof(float*) * mulitifield.xVals * mulitifield.yVals * mulitifield.zVals);
    int index = 0;
    for (int x = 0; x < mulitifield.xVals; ++x) {
    for (int y = 0; y < mulitifield.yVals; ++y) {
    for (int z = 0; z < mulitifield.zVals; ++z) {
        distanceField[index] = (float*)malloc(sizeof(float) * 4);
        distanceField[index][0] = static_cast<float>(x);
        distanceField[index][1] = static_cast<float>(y);
        distanceField[index][2] = static_cast<float>(z);
        distanceField[index][3] = -1.f;
        int indexInDataset = x+(mulitifield.xVals*(z+(mulitifield.zVals * y)));
        // calculate the distance form the current point ot the closest attribute vertex
        float distance = std::numeric_limits<float>::max();
        for (const AttributeVertex & vertex : vertices) {
            // calcuate distance from current vertex
            float euclidianDistanceSum = 0;
            for (const AttributeVertexValue & value : vertex.values) {
                float component = value.attribute->values[indexInDataset] - value.value;
                euclidianDistanceSum = component * component;
            }
            float euclidianDistance = sqrt(euclidianDistanceSum);
            if (euclidianDistance < distance) distance = euclidianDistance;
        }
        distanceField[index][3] = distance;
        index++;
    }}}
    return distanceField;
}