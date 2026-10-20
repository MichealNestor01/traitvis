#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

// Reverses the four bytes of every float. Attribute files are big-endian; the
// desktop reader and the browser provider both call this after the bytes are in memory.
void swapFloatEndianInPlace(std::span<float> values);

// set of functions used to read in datasets
[[nodiscard]] std::vector<float> readFloatBinaryFile(const std::filesystem::path& file, std::size_t count, bool reverseByteOrder = true);
