#include "shaderProgram.hpp"
#include "shaderSource.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <sstream>

GLuint loadShader(GLenum type, const char * path) {
    const std::string sourceStr = readShaderSource(path);
    std::vector<GLchar> sourceCode(sourceStr.begin(), sourceStr.end());

    GLuint shader = glCreateShader(type);
    sourceCode.push_back('\0'); // ensure the source code is null termintated
    const char * source = {sourceCode.data()};

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (not success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        glDeleteShader(shader);
        std::ostringstream msg;
        msg << "Unable to compile shader \"" << path << "\":" << infoLog;
        throw std::runtime_error(msg.str());
    }

    return shader;
}


ShaderProgram::ShaderProgram(std::vector<ShaderSource> sources)
    : program(glCreateProgram()) {
    std::vector<GLuint> shaders;
    shaders.reserve(sources.size());
    try {
        for (auto const& source: sources) {
            const GLuint shader = loadShader(source.type, source.path.c_str());
            glAttachShader(program.get(), shader);
            shaders.push_back(shader);
        }

        glLinkProgram(program.get());
        int success = 0;
        glGetProgramiv(program.get(), GL_LINK_STATUS, &success);
        if (not success) {
            char infoLog[512];
            glGetProgramInfoLog(program.get(), 512, NULL, infoLog);
            throw std::runtime_error(std::string("Shader program linking failed") + infoLog);
        }
    } catch (...) {
        for (const GLuint shader : shaders) glDeleteShader(shader);
        throw;
    }

    for (const GLuint shader : shaders) glDeleteShader(shader);
}

void ShaderProgram::setUniformb(std::string name, GLboolean b) {
    int location = glGetUniformLocation(program.get(), name.c_str());
    glUniform1i(location, b);
}

void ShaderProgram::setUniformi(std::string name, GLint value) {
    int location = glGetUniformLocation(program.get(), name.c_str());
    glUniform1i(location, value);
}

void ShaderProgram::setUniformf(std::string name, GLfloat f) {
    int location = glGetUniformLocation(program.get(), name.c_str());
    glUniform1f(location, f);
}

void ShaderProgram::setUniform3f(std::string name, GLfloat f1, GLfloat f2, GLfloat f3) {
    int location = glGetUniformLocation(program.get(), name.c_str());
    glUniform3f(location, f1, f2, f3);
}

void ShaderProgram::setUniform3f(std::string name, glm::vec3 f) {
    int location = glGetUniformLocation(program.get(), name.c_str());
    glUniform3f(location, f.x, f.y, f.z);
}

void ShaderProgram::setUniformMat4f(std::string name, glm::mat4 *mat) {
    int location = glGetUniformLocation(program.get(), name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(*mat));
}
