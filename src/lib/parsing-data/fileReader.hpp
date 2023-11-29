#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdint>

// set of functions used to read in datasets
std::vector<float> readFloatBinaryFile(const char* filename, int ndata);
std::vector<float> readFloatBinaryFile(std::string filename, int ndata);