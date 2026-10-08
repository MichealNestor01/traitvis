#pragma once
#include <glm.hpp>
#include <vector>
#include "../multiField/multiField.hpp"
#include "../marchingCubes/marchingCubes.hpp"
#include "distanceField.hpp"

class FeatureLevelSet {
public:
    std::vector<TraitPoint> points;
    Surface surface;
    glm::vec3 colour;
    float transparency = 1.f;
    bool active = true;
    float renderDepth = 1.f;
    std::string id;
    unsigned int VAO = 0;
    unsigned int activeInstanceVBO = 0;
    unsigned int inactiveInstanceVBO = 0;
    bool showActiveInactivePixels = false;
    bool invertNormals = false;
    
    FeatureLevelSet(std::vector<TraitPoint> points, MultiField & dataset, float euclideanDistance, glm::vec3 colour, std::string id);
    
    bool operator<(const FeatureLevelSet &rhs) const {
        return renderDepth < rhs.renderDepth;
    }
};