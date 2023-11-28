#pragma once 
#include <vector>

typedef struct floatRange {
    float upper;
    float lower;
};

typedef struct floatDomain {
    unsigned int dimensions;
    std::vector<floatRange> dimensionRanges;
    std::vector<float> values;
};

typedef struct multiField {
    floatDomain spacialDomain;
    floatDomain attributeDomain; 
};