#include "fileReader.hpp"

// endianess reverser from Gregor Brandt at https://stackoverflow.com/questions/2782725/converting-float-values-from-big-endian-to-little-endian
float reverseFloat( const float inFloat )
{
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}

std::vector<float> readFloatBinaryFile(const char* filename, int ndata, bool reverseByteOrder) {
    std::ifstream inputFile(filename, std::ios::binary);

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return {};  // Return an empty vector to indicate failure
    }

    std::vector<float> data(ndata);

    // Read binary data into the vector
    inputFile.read(reinterpret_cast<char*>(data.data()), sizeof(float) * ndata);

    // reverse byte order 
    if (reverseByteOrder) 
        std::transform(data.begin(), data.end(), data.begin(), [](float c){return reverseFloat(c);});

    // Close the file
    inputFile.close();

    return data;
}

std::vector<float> readFloatBinaryFile(std::string filename, int ndata, bool reverseByteOrder) {
    return readFloatBinaryFile(filename.c_str(), ndata, reverseByteOrder);
}