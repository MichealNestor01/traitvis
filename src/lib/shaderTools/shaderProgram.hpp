// This was heavily modified from Markus Bileter's COMP3811 Computer Graphics module.
#pragma once

#include "../../platform/gl.hpp"
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <vector>

#include "../util/uniqueHandle.hpp"

struct ProgramDeleter {
    void operator()(unsigned int id) const noexcept { glDeleteProgram(id); }
};

class ShaderProgram {
    public:
        struct ShaderSource {
            GLenum type;
            std::string path;
        };

        explicit ShaderProgram(std::vector<ShaderSource> sources);
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&&) noexcept = default;
        ShaderProgram& operator=(ShaderProgram&&) noexcept = default;

        UniqueHandle<ProgramDeleter> program;
        void setUniformb(std::string name, GLboolean b);
        void setUniformi(std::string name, GLint i);
        void setUniformf(std::string name, GLfloat f);
        void setUniform3f(std::string name, GLfloat f1, GLfloat f2, GLfloat f3);
        void setUniform3f(std::string name, glm::vec3 f);
        void setUniformMat4f(std::string name, glm::mat4 *mat);
};
