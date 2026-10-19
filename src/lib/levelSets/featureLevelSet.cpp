#include "featureLevelSet.hpp"

#include <utility>


FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, std::shared_ptr<const MultiField> dataset, float euclideanDistance, glm::vec3 colour, std::string id, bool keepField)
    : points(std::move(points)), colour(colour), id(std::move(id)), dataset(std::move(dataset)), isoValue(euclideanDistance) {
    for (TraitPoint& point : this->points) point.normalise();
    ScalarField distance = generateDistanceField(this->points, *this->dataset);
    surface = extractSurface(distance, isoValue);
    if (keepField) distanceField = std::move(distance);
}

bool FeatureLevelSet::reslice(float newIsoValue) {
    if (!distanceField) return false;
    isoValue = newIsoValue;
    surface = extractSurface(*distanceField, isoValue);
    mesh.reset();
    return true;
}

FeatureLevelSet::FeatureLevelSet(std::vector<TraitPoint> points, Surface surface, glm::vec3 colour, std::string id, std::shared_ptr<const MultiField> dataset)
    : points(std::move(points)), surface(std::move(surface)), colour(colour), id(std::move(id)), dataset(std::move(dataset)) {}