#pragma once

#include "../marchingCubes/marchingCubes.hpp"
#include "../util/uniqueHandle.hpp"

#include <span>

// The delete function is a pointer so this header can live in the GL-free core.
// uploadMesh, which is compiled with the application, installs the real GL delete.
struct VaoDeleter {
    void (*destroy)(unsigned int) = nullptr;
    void operator()(unsigned int id) const noexcept { if (destroy) destroy(id); }
};

struct BufferDeleter {
    void (*destroy)(unsigned int) = nullptr;
    void operator()(unsigned int id) const noexcept { if (destroy) destroy(id); }
};

// The buffers are declared first so they are destroyed after the vertex array.
struct GpuMesh {
    UniqueHandle<BufferDeleter> vbo;
    UniqueHandle<BufferDeleter> activeInstances;
    UniqueHandle<BufferDeleter> inactiveInstances;
    UniqueHandle<VaoDeleter> vao;
    int vertexCount = 0;
};

[[nodiscard]] GpuMesh uploadMesh(const Surface& surface);
[[nodiscard]] GpuMesh uploadMesh(std::span<const float> interleaved);
void bindInstanceBuffer(unsigned int instanceVBO);
