#include "datasetReader.hpp"

#include <string>
#include <vector>
#include <iostream>

#include "../multiField/multiField.hpp"
#include "datasetConfig.hpp"
#include "fileReader.hpp"
#include "configParser.hpp"


MultiField readAttributePerFileDataset(const DatasetDirConfig& config) {
    AttributePerFileDataset* dataset = (AttributePerFileDataset*) config.dataset;

    MultiField multiField;

    multiField.name = config.name;
    multiField.xVals = dataset->xVals;
    multiField.yVals = dataset->yVals;
    multiField.zVals = dataset->zVals;
    multiField.indexFunction = dataset->getIndexFunction();

    int valuesPerFile = dataset->getValuesPerFile();
    // setup the attributes
    for (auto file : dataset->getFiles()) {
        std::string path = config.filePath + file.filename;
        std::vector<float> vals = readFloatBinaryFile(path, valuesPerFile);
        if (vals.size() == 0) {
            std::cerr << "Failed to read values from " << path << std::endl;
            return multiField;
        }
        // reinterpret the lower and upper bounds as the actual lowest and greatest value in the file
        // values will then be normalised between these bounds
        float lowerBound = file.upperBound;
        float upperBound = file.lowerBound;
        for (float val : vals) {
            if (val != 1e35) {
                if (val < lowerBound && val >= file.lowerBound) lowerBound = val;
                if (val > upperBound && val <= file.upperBound) upperBound = val;
            }
        }

        multiField.attributeDomain.push_back({file.name, {lowerBound, upperBound}, vals});
    }

    // normalise the attributes
    multiField.normaliseAttributes();

    multiField.readError = false;
    return multiField;
}

MultiField readDataset(std::string filepath) {
    DatasetDirConfig config = parseConfig(filepath); 
    MultiField empty;

    if (config.parseError) {
        std::cerr << "Failed to parse dataset config from " << filepath << std::endl;
        return empty;
    }

    config.printConfig();

    if (config.dataset->format == ATTRIBUTEPERFILE) {
        return readAttributePerFileDataset(config);
    } 

    std::cerr  << "Dataset format not currently supported" << std::endl;
    return empty;
}

