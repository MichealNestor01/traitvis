#include "fileReader.hpp"

#include <algorithm>
#include <fstream>

// endianess reverser from Gregor Brandt at https://stackoverflow.com/questions/2782725/converting-float-values-from-big-endian-to-little-endian
float reverseFloat(float inFloat) {
   float retVal;
   char *floatToConvert = reinterpret_cast<char*>(&inFloat);
   char *returnFloat = reinterpret_cast<char*>(&retVal);

   // swap the bytes
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}

std::vector<float> readFloatBinaryFile(const std::filesystem::path& file, std::size_t count, bool reverseByteOrder) {
    std::ifstream inputFile(file, std::ios::binary);

    if (!inputFile.is_open()) return {};

    std::vector<float> data(count);

    inputFile.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(count * sizeof(float)));
    if (inputFile.gcount() != static_cast<std::streamsize>(count * sizeof(float))) return {};

    if (reverseByteOrder)
        std::transform(data.begin(), data.end(), data.begin(), [](float c){ return reverseFloat(c); });

    return data;
}
