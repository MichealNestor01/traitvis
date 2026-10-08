#include "datasetReader.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "../multiField/multiField.hpp"
#include "datasetConfig.hpp"
#include "fileReader.hpp"
#include "configParser.hpp"


MultiField readAttributePerFileDataset(const DatasetDirConfig& config) {
    AttributePerFileDataset* dataset = static_cast<AttributePerFileDataset*>(config.dataset.get());

    MultiField multiField;

    multiField.name = config.name;
    multiField.xVals = dataset->xVals;
    multiField.yVals = dataset->yVals;
    multiField.zVals = dataset->zVals;
    multiField.strides = makeStrides(dataset->scheme, dataset->dimOrder, dataset->xVals, dataset->yVals, dataset->zVals);

    int valuesPerFile = dataset->getValuesPerFile();
    // setup the attributes
    for (const auto& file : dataset->getFiles()) {
        const std::filesystem::path path = config.filePath / file.filename;
        std::vector<float> vals = readFloatBinaryFile(path, static_cast<std::size_t>(valuesPerFile), true);
        if (vals.size() == 0) {
            multiField.error = "Failed to read values from " + path.string() + "\n";
            return multiField;
        }
        // A configured fill value becomes NaN so it passes through normalisation
        // unchanged and loses every distance comparison (NaN < x is false).
        if (config.noData.has_value()) {
            const float sentinel = *config.noData;
            std::ranges::replace_if(vals,
                [sentinel](float value) { return value == sentinel; },
                std::numeric_limits<float>::quiet_NaN());
        }
        // reinterpret the lower and upper bounds as the actual lowest and greatest value in the file
        // values will then be normalised between these bounds
        float lowerBound = file.upperBound;
        float upperBound = file.lowerBound;
        for (float val : vals) {
            if (std::isnan(val)) continue;
            if (val < lowerBound && val >= file.lowerBound) lowerBound = val;
            if (val > upperBound && val <= file.upperBound) upperBound = val;
        }

        multiField.attributeDomain.push_back({file.name, {lowerBound, upperBound}, vals});
    }

    // normalise the attributes
    multiField.normaliseAttributes();
    return multiField;
}

MultiField readDataset(std::string filepath) {
    DatasetDirConfig config = parseConfig(filepath);
    if (!config.ok()) {
        MultiField failed;
        failed.error = config.error;
        return failed;
    }

    if (config.dataset->format == ATTRIBUTEPERFILE)
        return readAttributePerFileDataset(config);

    MultiField failed;
    failed.error = "Dataset format not currently supported\n";
    return failed;
}

