#pragma once 
#include <vector>
#include <string>

typedef struct floatRange {
    float lower;
    float upper;
} floatRange;

typedef struct attribute {
    std::string name;
    floatRange bounds;
    std::vector<float> values;
} attribute;

typedef struct floatDomainSpecification {
    unsigned int dimensions;
    std::vector<floatRange> dimensionRanges;
} floatDomainSpecification;

typedef struct multiField {
    floatDomainSpecification spacialDomain;
    std::vector<attribute> attributeDomain; 
} multiField;