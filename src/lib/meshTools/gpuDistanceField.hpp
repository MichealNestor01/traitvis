#pragma once

#include "../levelSets/distanceField.hpp"
#include "../shaderTools/shaderProgram.hpp"

#include <optional>
#include <span>

// GL 4.3 compute dispatch of the distance field. This is compiled into the desktop
// application, so the core library stays free of OpenGL. nullopt tells the caller
// to use the CPU slabs: there is no compute shader, the context is older than 4.3,
// or the field does not fit in a shader storage buffer.
class GpuDistanceField {
public:
    GpuDistanceField();
    [[nodiscard]] std::optional<ScalarField> compute(std::span<const TraitPoint> points, const MultiField& field);

private:
    std::optional<ShaderProgram> program;
};
