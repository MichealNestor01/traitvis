#pragma once 

#include "../multiField/multiField.hpp"
#include <string>
#include <vector>


typedef struct dimensionFile {
    std::string filename;
    floatRange bounds; 
};

typedef struct multiFieldDirConfig {
    int valuesPerFile;
    std::string dirPath;
    std::vector<dimensionFile> files; 
};