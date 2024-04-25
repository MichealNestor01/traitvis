#include <iostream>
#include "featureLevelSet.hpp"


FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> vertices, MultiField & dataset, float euclidianDistance, glm::vec3 colour, std::string id) {
    this->colour = colour;
    this->id = id;
    this->vertices = vertices;
    // first normalise the values of the vertices chosen
    for (TraitPoint & point : this->vertices) point.normalise();
    // create the distance field
    std::vector<std::vector<float>> distanceField = generateDistanceField(this->vertices, dataset);
    // extract the surface given the normalised eucliean distance
    surface = extractSurface(distanceField, {dataset.xVals, dataset.yVals, dataset.zVals}, euclidianDistance);
    surfaceVertices = surface.getVertices();
}