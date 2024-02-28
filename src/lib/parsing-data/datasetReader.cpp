#include "datasetReader.hpp"

#include <string>
#include <vector>
#include <iostream>

#include "../multiField/multiField.hpp"
#include "datasetConfig.hpp"
#include "fileReader.hpp"
#include "configParser.hpp"


MultiField readAttributePerFileDataset(DatasetDirConfig config) {
    AttributePerFileDataset* dataset = (AttributePerFileDataset*) config.dataset;

    MultiField multiField;

    multiField.xVals = dataset->xVals;
    multiField.yVals = dataset->yVals;
    multiField.zVals = dataset->zVals;

    // setup the attributes
    for (auto file : dataset->getFiles()) {
        std::string path = config.filePath + file.filename;
        std::vector<float> vals = readFloatBinaryFile(path, dataset->getValuesPerFile());
        if (vals.size() == 0) {
            std::cerr << "Failed to read values from " << path << std::endl;
            return multiField;
        }
        multiField.attributeDomain.push_back({file.name, {file.lowerBound, file.upperBound}, vals});
    }
    multiField.readError = false;
    return multiField;
}

MultiField readDataset(std::string filepath) {
    DatasetDirConfig config = parseConfig(filepath); 

    if (config.parseError) {
        std::cerr << "Failed to parse dataset config from " << filepath << std::endl;
        MultiField empty;
        return empty;
    }

    config.printConfig();

    if (config.dataset->format == ATTRIBUTEPERFILE) {
        return readAttributePerFileDataset(config);
    } 

    std::cerr  << "Dataset format not currently supported" << std::endl;
    return {};
}

