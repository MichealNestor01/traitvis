#pragma once
#include <atomic>
#include <optional>
#include <span>
#include <vector>
#include "../multiField/multiField.hpp"
#include "scalarField.hpp"

class DistanceFieldJob {
public:
    DistanceFieldJob(std::vector<TraitPoint> points, const MultiField& field);
    // Computes up to `slabCount` more x-slabs (in parallel where threads exist).
    // Returns true once the whole field is done. Calling step() on a finished job is a no-op.
    bool step(int slabCount);
    [[nodiscard]] bool done() const noexcept { return nextSlab >= result.layout.x; }
    [[nodiscard]] float progress() const noexcept;   // slabsDone / layout.x
    [[nodiscard]] ScalarField take() &&;             // only valid when done()
private:
    void computeSlab(int x);
    std::vector<TraitPoint> points;
    const MultiField& field;
    ScalarField result;
    int nextSlab = 0;
    std::atomic<int> slabsDone{0};
};

[[nodiscard]] ScalarField generateDistanceField(std::span<const TraitPoint> points, const MultiField& field);

// One trait value, in the same normalised units as the samples.
struct DistanceFieldComponent {
    int attribute = 0;
    float target = 0.f;
};

// A trait point is a contiguous run of components: [begin, begin + count).
struct DistanceFieldPoint {
    int begin = 0;
    int count = 0;
};

// The buffers the desktop compute shader reads. sampleCount is the length of every
// attribute. Distances are written in GridLayout order; samples are read with strides.
// nullopt means a trait points at an attribute that is not in this dataset, or the
// attributes disagree about how many samples they hold.
struct DistanceFieldDispatch {
    GridLayout layout;
    IndexStrides strides;
    int sampleCount = 0;
    std::vector<DistanceFieldComponent> components;
    std::vector<DistanceFieldPoint> points;
};

[[nodiscard]] std::optional<DistanceFieldDispatch> packDistanceField(std::span<const TraitPoint> points, const MultiField& field);
