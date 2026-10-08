#pragma once
#include <cstddef>
#include <glm.hpp>

unsigned int createVAO(const float vertices[], std::size_t size);
unsigned int createInstanceBuffer(const glm::mat4* matrices, std::size_t count);
void bindInstanceBuffer(unsigned int instanceVBO);
