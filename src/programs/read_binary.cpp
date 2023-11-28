#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "../parsing-data/reader.hpp"

int main(int argc, char ** argv) {
    if (argc < 3) {
        std::cout << "USAGE: ./reader file.bin <number of data points>" << std::endl;
        return 0;
    }

    std::vector<float> cloud_data = readFloatBinaryFile(argv[1], atoi(argv[2]));
    if (cloud_data.empty()) return 1;

    std::cout << "Successfully read " << cloud_data.size() << " data points from " << argv[1] << std::endl;

    //std::cout << "Datapoint 24999999: " << cloud_data.at(24999999) << std::endl;

    /*
    for (const float& datapoint : cloud_data ) {
        std::cout << "Read datapoint: " << datapoint << std::endl;
    }
    */
    
    return 0;
}