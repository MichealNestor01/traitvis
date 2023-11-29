#include "dirReader.hpp"

multiField readIsabel(multiFieldDirConfig config) {
    // setup spacial domain 
    floatDomainSpecification spacialDomain = {
        /*dimensions*/ 3,
        /*ranges*/ {
            /*x*/ {0,499}, 
            /*y*/ {0,499}, 
            /*z*/ {0,99}
        }
    };

    // setup attribute domain
    std::vector<attribute> attributeDomain = {};

    // fill the attribute domain based on the config 
    for (int fileIndex = 0; fileIndex < config.files.size(); fileIndex++) {
        std::string filename = config.files[fileIndex].filename;
        std::string path = config.dirPath + filename;
        std::vector<float> vals = readFloatBinaryFile(path, config.valuesPerFile);
        attributeDomain.push_back({
            /*name (without f0n.bin)*/ filename.substr(0, filename.length() - 7),
            /*bounds*/ config.files[fileIndex].bounds,
            /*values*/ vals
        });
    }

    return {spacialDomain, attributeDomain};
}