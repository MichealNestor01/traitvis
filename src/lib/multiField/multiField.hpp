#pragma once 
#include <vector>
#include <string>

struct FloatRange {
    float lower;
    float upper;
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

struct IndexStrides {
    int x, y, z;
    [[nodiscard]] constexpr int index(int ix, int iy, int iz) const noexcept {
        return ix * x + iy * y + iz * z;
    }
};

struct MultiField {
    bool readError = true;
    std::string name;
    int xVals, yVals, zVals;
    std::vector<Attribute> attributeDomain;
    IndexStrides strides;

    void normaliseAttributes() {
        for (auto &attribute : attributeDomain) {
            float range = attribute.bounds.upper - attribute.bounds.lower;
            if (range == 0) continue;
            for (float &value : attribute.values) value = (value - attribute.bounds.lower) / range;
        }
    }

    int getIndexInDataset(int x, int y, int z) const {
        return strides.index(x, y, z);
    }
};