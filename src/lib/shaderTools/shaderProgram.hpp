// This was heavily modified from Markus Bileter's COMP3811 Computer Graphics module.
#pragma once 

#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <vector>

class ShaderProgram {
    public:
        struct ShaderSource {
            GLenum type;
            std::string path;
        };
        ShaderProgram(std::vector<ShaderSource> = {});
        ~ShaderProgram();
        GLuint program;
        void setUniform3f(std::string name, float f1, float f2, float f3);
        void setUniform3f(std::string name, glm::vec3 f);
        void setUniformMat4f(std::string name, glm::mat4 *mat);
};