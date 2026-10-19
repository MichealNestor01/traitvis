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
    // The isovalue the current surface was extracted at. Meaningful when distanceField is set.
    float isoValue = 0.f;
    // Present only when the user asked to keep it. Isabel's field is about 100 MB, so this is opt-in.
    std::optional<ScalarField> distanceField;

    FeatureLevelSet(std::vector<TraitPoint> points, std::shared_ptr<const MultiField> dataset, float euclideanDistance, glm::vec3 colour, std::string id, bool keepField = false);
    FeatureLevelSet(std::vector<TraitPoint> points, Surface surface, glm::vec3 colour, std::string id, std::shared_ptr<const MultiField> dataset);
    // Extracts a new surface from the kept field. Drops the uploaded mesh so the next draw uploads the new one.
    // Returns false when no field was kept.
    bool reslice(float newIsoValue);
};