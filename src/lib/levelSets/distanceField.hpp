#pragma once
#include <atomic>
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
