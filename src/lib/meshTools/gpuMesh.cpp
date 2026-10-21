#include "gpuMesh.hpp"

#include "../../platform/gl.hpp"
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include <vector>

namespace {

void deleteVao(unsigned int id) {
    glDeleteVertexArrays(1, &id);
}

void deleteBuffer(unsigned int id) {
    glDeleteBuffers(1, &id);
}

UniqueHandle<BufferDeleter> uploadInstances(const std::set<glm::vec3, Vec3Comparator>& vertices) {
    if (vertices.empty()) return {};
    std::vector<glm::mat4> models;
    models.reserve(vertices.size());
    for (const glm::vec3& vertex : vertices)
        models.push_back(glm::scale(glm::translate(glm::mat4(1.f), vertex), glm::vec3(0.1f)));

    unsigned int id = 0;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(models.size() * sizeof(glm::mat4)), models.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return UniqueHandle<BufferDeleter>(id, BufferDeleter{deleteBuffer});
}

}

GpuMesh uploadMesh(std::span<const float> interleaved) {
    unsigned int vbo = 0;
    unsigned int vao = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(interleaved.size() * sizeof(float)), interleaved.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GpuMesh mesh;
    mesh.vbo = UniqueHandle<BufferDeleter>(vbo, BufferDeleter{deleteBuffer});
    mesh.vao = UniqueHandle<VaoDeleter>(vao, VaoDeleter{deleteVao});
    mesh.vertexCount = static_cast<int>(interleaved.size() / 6);
    return mesh;
}

GpuMesh uploadMesh(const Surface& surface) {
    GpuMesh mesh = uploadMesh(std::span<const float>(surface.interleaved()));
    mesh.activeInstances = uploadInstances(surface.activeVertices);
    mesh.inactiveInstances = uploadInstances(surface.inactiveVertices);
    return mesh;
}

void bindInstanceBuffer(unsigned int instanceVBO) {
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    const auto stride = sizeof(glm::mat4);
    for (unsigned int column = 0; column < 4; ++column) {
        const unsigned int location = 2 + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(glm::vec4) * column));
        glVertexAttribDivisor(location, 1);
    }
}
