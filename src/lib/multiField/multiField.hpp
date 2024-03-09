#pragma once 
#include <vector>
#include <string>

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

struct AttributeVertexValue {
    Attribute * attribute;
    float value;
};

struct AttributeVertex {
    std::vector<AttributeVertexValue> values;

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

    void normaliseAttributes() {
        for (auto &attribute : attributeDomain) {
            float range = attribute.bounds.upper - attribute.bounds.lower;
            for (float &value : attribute.values) 
                value = (value - attribute.bounds.lower) / range;
        }
    }
};