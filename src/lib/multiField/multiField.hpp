#pragma once 
#include <vector>
#include <string>
#include <functional>

struct FloatRange {
    float lower;
    float upper;
};

struct IntRange {
    int lower;
    int upper;
};

struct Attribute {
    std::string name;
    FloatRange bounds;
    std::vector<float> values;
};

struct TraitPointComponent {
    Attribute * attribute;
    float value;
};

struct TraitPoint {
    std::vector<TraitPointComponent> values;

    void normalise() {
        for (auto &value : values) {
            float range = value.attribute->bounds.upper - value.attribute->bounds.lower;
            value.value = (value.value - value.attribute->bounds.lower) / range;
        }
    }
};

struct MultiField {
    bool readError = true;
    std::string name;
    int xVals, yVals, zVals;
    std::vector<Attribute> attributeDomain; 
    std::function<int(int, int, int, int, int, int)> indexFunction;

    void normaliseAttributes() {
        for (auto &attribute : attributeDomain) {
            float range = attribute.bounds.upper - attribute.bounds.lower;
            for (float &value : attribute.values) 
                value = (value - attribute.bounds.lower) / range;
        }
    }

    int getIndexInDataset(int x, int y, int z) const {
        return indexFunction(x, y, z, xVals, yVals, zVals);
    }
};