#include "featureLevelSet.hpp"

#include <utility>


FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, MultiField & dataset, float euclideanDistance, glm::vec3 colour, std::string id) {
    this->colour = colour;
    this->id = id;
    this->points = points;
    // first normalise the values of the points chosen
    for (TraitPoint & point : this->points) point.normalise();
    // create the distance field
    ScalarField distanceField = generateDistanceField(this->points, dataset);
    // extract the surface given the normalised eucliean distance
    surface = extractSurface(distanceField, euclideanDistance);
}

FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, Surface surface, glm::vec3 colour, std::string id)
    : points(std::move(points)), surface(std::move(surface)), colour(colour), id(std::move(id)) {}