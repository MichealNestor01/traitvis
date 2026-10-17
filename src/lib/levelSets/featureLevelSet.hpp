#pragma once
#include <glm.hpp>
#include <memory>
#include <optional>
#include <vector>
#include "../multiField/multiField.hpp"
#include "../marchingCubes/marchingCubes.hpp"
#include "../meshTools/gpuMesh.hpp"
#include "distanceField.hpp"

class FeatureLevelSet {
public:
    std::vector<TraitPoint> points;
    Surface surface;
    glm::vec3 colour;
    float transparency = 1.f;
    bool active = true;
    std::string id;
    // Keeps the attributes alive after this dataset is removed from the session.
    std::shared_ptr<const MultiField> dataset;
    // Filled on the main thread the first time the set is drawn. Empty until then.
    std::optional<GpuMesh> mesh;
    bool showActiveInactivePixels = false;
    bool invertNormals = false;
    
    FeatureLevelSet(std::vector<TraitPoint> points, std::shared_ptr<const MultiField> dataset, float euclideanDistance, glm::vec3 colour, std::string id);
    FeatureLevelSet(std::vector<TraitPoint> points, Surface surface, glm::vec3 colour, std::string id, std::shared_ptr<const MultiField> dataset);
};