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

typedef struct floatDomainSpecification {
    unsigned int dimensions;
    std::vector<floatRange> dimensionRanges;
};

typedef struct multiField {
    floatDomainSpecification spacialDomain;
    floatDomain attributeDomain; 
};