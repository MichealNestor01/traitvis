#include "featureLevelSet.hpp"

#include <utility>


FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, std::shared_ptr<const MultiField> dataset, float euclideanDistance, glm::vec3 colour, std::string id)
    : points(std::move(points)), colour(colour), id(std::move(id)), dataset(std::move(dataset)) {
    for (TraitPoint& point : this->points) point.normalise();
    ScalarField distanceField = generateDistanceField(this->points, *this->dataset);
    surface = extractSurface(distanceField, euclideanDistance);
}

FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, Surface surface, glm::vec3 colour, std::string id, std::shared_ptr<const MultiField> dataset)
    : points(std::move(points)), surface(std::move(surface)), colour(colour), id(std::move(id)), dataset(std::move(dataset)) {}