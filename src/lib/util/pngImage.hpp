#pragma once

#include <span>
#include <string>

// Writes an 8-bit RGB PNG. `rgb` is width * height * 3 bytes, row-major, top row first.
// Returns false when the size does not match or the file cannot be written.
[[nodiscard]] bool writePng(const std::string& path, int width, int height, std::span<const unsigned char> rgb);
