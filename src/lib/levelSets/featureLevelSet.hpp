#pragma once

#include <glm.hpp>
#include <vector>
#include "../multiField/multiField.hpp"
#include "../marchingCubes/marchingCubes.hpp"

class FeatureLevelSet {
private:
    float ** generateDistanceField(const std::vector<AttributeVertex> & vertices, const MultiField & mulitifield);

public:
    std::vector<AttributeVertex> vertices;
    Triangles surface;
    std::vector<float> surfaceVertices;
    glm::vec3 colour;
    float transparency = 1.f;
    bool active = true;
    float renderDepth = 1.f;
    std::string id;
    unsigned int VAO = 0;
    bool showActiveInactivePixels = false;
    
    FeatureLevelSet(std::vector<AttributeVertex> vertices, MultiField & dataset, float euclidianDistance, glm::vec3 colour, std::string id);
    
    bool operator<(const FeatureLevelSet &rhs) const {
        return renderDepth < rhs.renderDepth;
    }
};