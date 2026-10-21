#include "gpuDistanceField.hpp"

#include "../util/uniqueHandle.hpp"

#include "../../platform/gl.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#ifndef __EMSCRIPTEN__

namespace {

// assets/distanceField.comp layout(local_size_x = 256).
constexpr int kLocalSize = 256;

struct StorageDeleter {
    void operator()(unsigned int id) const noexcept { glDeleteBuffers(1, &id); }
};

using StorageBuffer = UniqueHandle<StorageDeleter>;

static_assert(sizeof(DistanceFieldComponent) == 8);
static_assert(alignof(DistanceFieldComponent) == 4);
static_assert(sizeof(DistanceFieldPoint) == 8);
static_assert(alignof(DistanceFieldPoint) == 4);

bool glOk() {
    bool ok = true;
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) ok = false;
    return ok;
}

bool computeShadersAvailable() {
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    (void)glOk();
    return major > 4 || (major == 4 && minor >= 3);
}

StorageBuffer upload(std::size_t bytes, const void* data) {
    unsigned char dummy[4] = {};
    if (bytes == 0) {
        bytes = sizeof(dummy);
        data = dummy;
    }
    unsigned int id = 0;
    glGenBuffers(1, &id);
    StorageBuffer buffer(id);
    if (!buffer) return buffer;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer.get());
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(bytes), data, GL_DYNAMIC_DRAW);
    return buffer;
}

bool fitsInStorage(const MultiField& field, int sampleCount, int voxels) {
    GLint64 maxBlock = 0;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxBlock);
    if (maxBlock <= 0) return false;
    const auto attributes = static_cast<std::uint64_t>(field.attributeDomain.size());
    const auto samples = static_cast<std::uint64_t>(std::max(sampleCount, 0));
    // The shader indexes the value buffer with a 32-bit unsigned product.
    if (samples > 0 && attributes > std::numeric_limits<std::uint32_t>::max() / samples) return false;
    const std::uint64_t valueBytes = attributes * samples * sizeof(float);
    const std::uint64_t distanceBytes = static_cast<std::uint64_t>(voxels) * sizeof(float);
    return valueBytes <= static_cast<std::uint64_t>(maxBlock) && distanceBytes <= static_cast<std::uint64_t>(maxBlock);
}

bool dispatchShape(int voxels, int& groupsX, int& groupsY) {
    GLint maxX = 0;
    GLint maxY = 0;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxX);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxY);
    if (maxX < 1 || maxY < 1) return false;
    const int groups = (voxels + kLocalSize - 1) / kLocalSize;
    groupsX = std::min(groups, maxX);
    groupsY = (groups + groupsX - 1) / groupsX;
    if (groupsY > maxY) return false;
    return static_cast<std::uint64_t>(groupsX) * static_cast<unsigned>(kLocalSize) <= std::numeric_limits<std::uint32_t>::max();
}

bool setInt(unsigned int shader, const char* name, int value) {
    const GLint location = glGetUniformLocation(shader, name);
    if (location < 0) return false;
    glUniform1i(location, value);
    return true;
}

bool setIvec3(unsigned int shader, const char* name, int x, int y, int z) {
    const GLint location = glGetUniformLocation(shader, name);
    if (location < 0) return false;
    glUniform3i(location, x, y, z);
    return true;
}

}

GpuDistanceField::GpuDistanceField() {
    program.emplace(std::vector<ShaderProgram::ShaderSource>{
        {GL_COMPUTE_SHADER, "assets/distanceField.comp"}
    });
}

std::optional<ScalarField> GpuDistanceField::compute(std::span<const TraitPoint> points, const MultiField& field) {
    if (!program || !computeShadersAvailable()) return std::nullopt;

    const std::optional<DistanceFieldDispatch> dispatch = packDistanceField(points, field);
    if (!dispatch) return std::nullopt;
    const GridLayout& layout = dispatch->layout;
    if (layout.x < 0 || layout.y < 0 || layout.z < 0) return std::nullopt;
    const int voxels = layout.total();
    if (voxels == 0) return ScalarField{layout, {}};
    if (voxels < 0 || !fitsInStorage(field, dispatch->sampleCount, voxels)) {
        (void)glOk();
        return std::nullopt;
    }

    int groupsX = 0;
    int groupsY = 0;
    if (!dispatchShape(voxels, groupsX, groupsY)) {
        (void)glOk();
        return std::nullopt;
    }

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    struct RestoreProgram {
        GLint previous;
        ~RestoreProgram() { glUseProgram(static_cast<GLuint>(previous)); }
    } restore{previousProgram};
    (void)restore;

    const unsigned int shader = program->program.get();
    glUseProgram(shader);
    const bool uniforms =
        setInt(shader, "voxelCount", voxels) &&
        setIvec3(shader, "gridSize", layout.x, layout.y, layout.z) &&
        setIvec3(shader, "strides", dispatch->strides.x, dispatch->strides.y, dispatch->strides.z) &&
        setInt(shader, "sampleCount", dispatch->sampleCount) &&
        setInt(shader, "pointCount", static_cast<int>(dispatch->points.size()));
    if (!uniforms) return std::nullopt;

    const std::size_t valueBytes = field.attributeDomain.size() * static_cast<std::size_t>(std::max(dispatch->sampleCount, 0)) * sizeof(float);
    StorageBuffer values = upload(valueBytes, nullptr);
    StorageBuffer components = upload(dispatch->components.size() * sizeof(DistanceFieldComponent), dispatch->components.data());
    StorageBuffer traitPoints = upload(dispatch->points.size() * sizeof(DistanceFieldPoint), dispatch->points.data());
    StorageBuffer distances = upload(static_cast<std::size_t>(voxels) * sizeof(float), nullptr);
    struct UnbindStorage {
        ~UnbindStorage() {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            for (int binding = 0; binding < 4; ++binding)
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0);
        }
    } unbind;
    (void)unbind;
    if (!values || !components || !traitPoints || !distances) return std::nullopt;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, values.get());
    std::size_t offset = 0;
    for (const Attribute& attribute : field.attributeDomain) {
        const std::size_t attributeBytes = attribute.values.size() * sizeof(float);
        if (attributeBytes == 0) continue;
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(attributeBytes), attribute.values.data());
        offset += attributeBytes;
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, values.get());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, components.get());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, traitPoints.get());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, distances.get());
    glDispatchCompute(static_cast<GLuint>(groupsX), static_cast<GLuint>(groupsY), 1);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    if (!glOk()) return std::nullopt;

    ScalarField result;
    result.layout = layout;
    result.values.resize(static_cast<std::size_t>(voxels));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, distances.get());
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(result.values.size() * sizeof(float)), result.values.data());
    if (!glOk()) return std::nullopt;
    return result;
}

#else

GpuDistanceField::GpuDistanceField() = default;

std::optional<ScalarField> GpuDistanceField::compute(std::span<const TraitPoint>, const MultiField&) {
    return std::nullopt;
}

#endif
