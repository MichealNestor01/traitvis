#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include "../util/parallelFor.hpp"
#include "distanceField.hpp"

DistanceFieldJob::DistanceFieldJob(std::vector<TraitPoint> points, const MultiField& field)
    : points(std::move(points)), field(field) {
    result.layout = {this->field.xVals, this->field.yVals, this->field.zVals};
    result.values.assign(static_cast<std::size_t>(result.layout.total()), 0.f);
}

bool DistanceFieldJob::step(int slabCount) {
    const int remaining = result.layout.x - nextSlab;
    const int count = std::min(slabCount, std::max(remaining, 0));
    const int end = nextSlab + count;
    parallelFor(nextSlab, end, [this](int x) {
        computeSlab(x);
        ++slabsDone;
    });
    nextSlab = end;
    return done();
}

float DistanceFieldJob::progress() const noexcept {
    const int slabs = result.layout.x;
    if (slabs == 0) return 1.f;
    return static_cast<float>(slabsDone.load()) / static_cast<float>(slabs);
}

ScalarField DistanceFieldJob::take() && {
    return std::move(result);
}

void DistanceFieldJob::computeSlab(int x) {
    for (int y = 0; y < result.layout.y; ++y) {
    for (int z = 0; z < result.layout.z; ++z) {
        const int indexInDataset = field.getIndexInDataset(x, y, z);
        // Squared length keeps the same nearest point as the Euclidean length, so the
        // square root is taken once. A partial sum that is already no closer can stop.
        // Summing every component and then comparing is the same result: a later square
        // cannot shrink the sum, and a non-finite component rejects the point either way.
        // The compute shader uses that form.
        float minDistSq = std::numeric_limits<float>::infinity();
        for (const TraitPoint& vertex : points) {
            float sum = 0.f;
            bool rejected = false;
            for (const TraitPointComponent& value : vertex.values) {
                const float component = value.attribute->values[indexInDataset] - value.value;
                const float squared = component * component;
                if (!std::isfinite(squared)) {
                    rejected = true;
                    break;
                }
                sum += squared;
                if (sum >= minDistSq) {
                    rejected = true;
                    break;
                }
            }
            if (!rejected && sum < minDistSq) minDistSq = sum;
        }
        // No finite sample (NaN data, or no trait points) stays at the distance-field sentinel.
        const float distance = std::isfinite(minDistSq)
            ? std::sqrt(minDistSq)
            : std::numeric_limits<float>::max();
        result.values[result.layout.index(x, y, z)] = distance;
    }}
}

ScalarField generateDistanceField(std::span<const TraitPoint> points, const MultiField& field) {
    DistanceFieldJob job({points.begin(), points.end()}, field);
    while (!job.step(std::numeric_limits<int>::max())) {}
    return std::move(job).take();
}

std::optional<DistanceFieldDispatch> packDistanceField(std::span<const TraitPoint> points, const MultiField& field) {
    DistanceFieldDispatch dispatch;
    dispatch.layout = {field.xVals, field.yVals, field.zVals};
    dispatch.strides = field.strides;

    if (!field.attributeDomain.empty()) {
        const std::size_t samples = field.attributeDomain[0].values.size();
        if (samples > static_cast<std::size_t>(std::numeric_limits<int>::max())) return std::nullopt;
        for (const Attribute& attribute : field.attributeDomain)
            if (attribute.values.size() != samples) return std::nullopt;
        dispatch.sampleCount = static_cast<int>(samples);
    }

    const auto indexOf = [&field](const Attribute* attribute) {
        if (attribute == nullptr) return -1;
        for (int i = 0; i < static_cast<int>(field.attributeDomain.size()); ++i)
            if (&field.attributeDomain[i] == attribute) return i;
        return -1;
    };

    for (const TraitPoint& point : points) {
        // An empty attribute array has nothing a component can read.
        if (dispatch.sampleCount == 0 && !point.values.empty()) return std::nullopt;
        const int begin = static_cast<int>(dispatch.components.size());
        for (const TraitPointComponent& component : point.values) {
            const int attribute = indexOf(component.attribute);
            if (attribute < 0) return std::nullopt;
            dispatch.components.push_back(DistanceFieldComponent{attribute, component.value});
        }
        dispatch.points.push_back(DistanceFieldPoint{begin, static_cast<int>(point.values.size())});
    }
    return dispatch;
}
