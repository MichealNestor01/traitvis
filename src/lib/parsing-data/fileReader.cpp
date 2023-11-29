#include "fileReader.hpp"

std::vector<float> readFloatBinaryFile(std::string filename, int ndata) {
    return readFloatBinaryFile(filename.c_str(), ndata);
}

std::vector<float> readFloatBinaryFile(const char* filename, int ndata) {
    std::ifstream inputFile(filename, std::ios::binary);

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return {};  // Return an empty vector to indicate failure
    }

    std::vector<float> data(ndata);

    // Read binary data into the vector
    inputFile.read(reinterpret_cast<char*>(data.data()), sizeof(float) * ndata);

    // Close the file
    inputFile.close();

    return data;
}