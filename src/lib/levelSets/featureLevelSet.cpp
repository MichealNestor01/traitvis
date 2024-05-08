#include <iostream>
#include "featureLevelSet.hpp"


FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, MultiField & dataset, float euclideanDistance, glm::vec3 colour, std::string id) {
    this->colour = colour;
    this->id = id;
    this->points = points;
    // first normalise the values of the points chosen
    for (TraitPoint & point : this->points) point.normalise();
    // create the distance field
    std::vector<std::vector<float>> distanceField = generateDistanceField(this->points, dataset);
    // extract the surface given the normalised eucliean distance
    surface = extractSurface(distanceField, {dataset.xVals, dataset.yVals, dataset.zVals}, euclideanDistance);
    surfaceVertices = surface.getVertices();
}