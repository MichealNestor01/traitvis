#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

// set of functions used to read in datasets
[[nodiscard]] std::vector<float> readFloatBinaryFile(const std::filesystem::path& file, std::size_t count, bool reverseByteOrder = true);
