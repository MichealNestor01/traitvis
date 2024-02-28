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

struct MultiField {
    bool readError = true;
    int xVals, yVals, zVals;
    std::vector<Attribute> attributeDomain; 
};