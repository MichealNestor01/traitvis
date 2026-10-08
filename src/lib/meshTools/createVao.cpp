#include "createVao.hpp"
#include <glad/glad.h>
#include <glm.hpp>

unsigned int createVAO(const float vertices[], std::size_t size) {
    unsigned int VAO, VBO;

    // setup VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    // setup VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // reset state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);

    return VAO;
}

unsigned int createInstanceBuffer(const glm::mat4* matrices, std::size_t count) {
    if (count == 0) return 0;
    unsigned int vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::mat4), matrices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
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
