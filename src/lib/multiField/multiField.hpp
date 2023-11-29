#pragma once 
#include <vector>

typedef struct floatRange {
    float lower;
    float upper;
} floatRange;

typedef struct floatDomain {
    unsigned int dimensions;
    std::vector<floatRange> dimensionRanges;
    std::vector<float> values;
} floatDomain;

typedef struct floatDomainSpecification {
    unsigned int dimensions;
    std::vector<floatRange> dimensionRanges;
} floatDomainSpecification;

typedef struct multiField {
    floatDomainSpecification spacialDomain;
    floatDomain attributeDomain; 
} multiField;