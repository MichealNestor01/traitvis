#pragma once

#include <string>

// Reads an entire file into a string (used for GLSL shader sources).
// Throws std::runtime_error if the file cannot be opened or fully read.
// Deliberately free of OpenGL includes so it can be unit tested headlessly.
std::string readShaderSource(const std::string& path);
