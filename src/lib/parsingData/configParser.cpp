#include "configParser.hpp"
#include "datasetConfig.hpp"
#include <filesystem>
#include <string>
#include <fstream>
#include <stdexcept>
#include <optional>

std::optional<IndexScheme> stringToIndexScheme(const std::string& str) {
    if (str == "ROWMAJOR") return ROWMAJOR;
    if (str == "COLUMNMAJOR") return COLUMNMAJOR;
    return std::nullopt;
}

std::optional<DimensionOrder> stringToDimensionOrder(const std::string& str) {
    if (str == "WIDTH_HEIGHT_DEPTH") return WIDTH_HEIGHT_DEPTH;
    if (str == "WIDTH_DEPTH_HEIGHT") return WIDTH_DEPTH_HEIGHT;
    return std::nullopt;
}


DatasetDirConfig parseConfig(std::string filepath) {
    std::ifstream configFile(filepath);
    DatasetDirConfig config;
    auto fail = [&](std::string message) {
        config.error = std::move(message);
        return std::move(config);
    };

    if (!configFile.is_open())
        return fail("Failed to open \"" + filepath + "\"\n");

    // read the name 
    std::string line;
    std::getline(configFile, line);
    if (line.substr(0,5) == "NAME:") {
        try {
            config.name = line.substr(5);
        } catch (const std::out_of_range& err) {
            return fail("Failed to parse NAME in \"" + filepath + "\"\n");
        }
    } else {
        return fail("Failed to find NAME in \"" + filepath + "\"\n");
    }

    // read the FILEPATH
    std::getline(configFile, line);
    if (line.substr(0,9) == "FILEPATH:") {
        try {
            const std::filesystem::path configDir = std::filesystem::path(filepath).parent_path();
            // An absolute FILEPATH replaces configDir; a relative one is resolved against it.
            config.filePath = std::filesystem::absolute(configDir / line.substr(9));
        } catch (const std::out_of_range& err) {
            return fail("Failed to parse FILEPATH in \"" + filepath + "\"\n");
        }
    } else {
        return fail("Failed to find FILEPATH in \"" + filepath + "\"\n");
    }

    // read the xVals, yVals and zVals from the file, in the format "SPATIALDOMAIN:xvals:yvals:zvals"
    int xVals, yVals, zVals;
    std::getline(configFile, line);
    if (line.substr(0,14) == "SPATIALDOMAIN:") {
        std::string spatialDomain = line.substr(14);

        auto firstColon = spatialDomain.find(':');
        if (firstColon == std::string::npos)
            return fail("Spatial domain formatting error in \"" + filepath + "\"\n");
        std::string xValsStr = spatialDomain.substr(0, firstColon);
        spatialDomain = spatialDomain.substr(firstColon + 1);

        auto secondColon = spatialDomain.find(':');
        if (secondColon == std::string::npos)
            return fail("Spatial domain formatting error in \"" + filepath + "\"\n");
        std::string yValsStr = spatialDomain.substr(0, secondColon);
        std::string zValsStr = spatialDomain.substr(secondColon + 1);

        try {
            xVals = std::stoi(xValsStr);
            yVals = std::stoi(yValsStr);
            zVals = std::stoi(zValsStr);
        } catch (const std::invalid_argument& err) {
            return fail("Failed to parse SPATIALDOMAIN values from \"" + filepath + "\": Invalid Integer\n");
        } catch (const std::out_of_range& err) {
            return fail("Failed to parse SPATIALDOMAIN values from  \"" + filepath + "\": Out of Integer range\n");
        }
    } else {
        return fail("Failed to find SPATIALDOMAIN in \"" + filepath + "\"\n");
    }

    DimensionOrder dimOrder;
    std::getline(configFile, line);
    if (line.substr(0,15) == "DIMENSIONORDER:") {
        try {
            std::string dimensionOrder = line.substr(15);
            auto orderOpt = stringToDimensionOrder(dimensionOrder);
            if (orderOpt) {
                dimOrder = orderOpt.value();
            } else {
                return fail("Error: DIMENSIONORDER option \"" + dimensionOrder + "\" is not supported\n");
            }
        } catch (const std::out_of_range& err) {
            return fail("Failed to parse DIMENSIONORDER in \"" + filepath + "\"\n");
        }
    } else {
        return fail("Failed to find DIMENSIONORDER in \"" + filepath + "\"\n");
    }
    
    IndexScheme scheme;
    std::getline(configFile, line);
    if (line.substr(0,12) == "INDEXSCHEME:") {
        try {
            std::string indexScheme = line.substr(12);
            auto schemeOpt = stringToIndexScheme(indexScheme);
            if (schemeOpt) {
                scheme = schemeOpt.value();
            } else {
                return fail("Error: INDEXSCHEME scheme \"" + indexScheme + "\" is not supported\n");
            }
        } catch (const std::out_of_range& err) {
            return fail("Failed to parse INDEXSCHEME in \"" + filepath + "\"\n");
        }
    } else {
        return fail("Failed to find INDEXSCHEME in \"" + filepath + "\"\n");
    }
    
    // Optional fill value. Omitted datasets keep every sample.
    std::getline(configFile, line);
    if (line.substr(0, 7) == "NODATA:") {
        try {
            config.noData = std::stof(line.substr(7));
        } catch (const std::invalid_argument&) {
            return fail("Failed to parse NODATA value from \"" + filepath + "\": Invalid float\n");
        } catch (const std::out_of_range&) {
            return fail("Failed to parse NODATA value from \"" + filepath + "\": Out of float range\n");
        }
        std::getline(configFile, line);
    }

    // read the datasetStructure
    std::string datasetType;
    std::string datasetStructure;
    if (line.substr(0,17) == "DATASETSTRUCTURE:") {
        try {
            datasetStructure = line.substr(17);
            datasetType = datasetStructure.substr(0, datasetStructure.find(":"));
        } catch (const std::out_of_range& err) {
            return fail("DATASTRUCTURE not found in \"" + filepath + "\"\n");
        }
    } else {
        return fail("Failed to find DATASETSTRUCTURE in \"" + filepath + "\"\n");
    }

    if (datasetType == "ATTRIBUTEPERFILE") {
        try {
            int valuesPerFile = std::stoi(datasetStructure.substr(datasetStructure.find(":")+1));
            config.dataset = std::make_unique<AttributePerFileDataset>(valuesPerFile, xVals, yVals, zVals, dimOrder, scheme);
            auto* dataset = static_cast<AttributePerFileDataset*>(config.dataset.get());

            // attributes will be listed as such in the file: ATTRIBUTE:filename:attributename:minfloatval:maxfloatval
            while (std::getline(configFile, line)) {
                if (line.substr(0,10) == "ATTRIBUTE:") {
                    try {
                        std::string attribute = line.substr(10);
                        std::string filename = attribute.substr(0, attribute.find(":"));
                        attribute = attribute.substr(attribute.find(":")+1);
                        std::string name = attribute.substr(0, attribute.find(":"));
                        attribute = attribute.substr(attribute.find(":")+1);
                        // throw an error if any of the strings are empty
                        if (filename.empty() || name.empty() || attribute.empty())
                            return fail("Attribute misformatted in \"" + filepath + "\": " + line + "\n");
                        try {
                            float lowerBound = std::stof(attribute.substr(0, attribute.find(":")));
                            float upperBound = std::stof(attribute.substr(attribute.find(":")+1));
                            dataset->addFile(filename, name, lowerBound, upperBound);
                        } catch (const std::invalid_argument& err) {
                            return fail("Unable to parse bounds for attribute: " + attribute + "\n");
                        } catch (const std::out_of_range& err) {
                            return fail("Bounds out of float value range for attribute: " + attribute + "\n");
                        }
                    } catch (const std::out_of_range& err) {
                        return fail("Attribute misformatted in \"" + filepath + "\": " + line + "\n");
                    }
                } else {
                    return fail("Failed to find ATTRIBUTE where expected in \"" + filepath + "\"\n");
                }
            }
        } catch (const std::invalid_argument& err) {
            return fail("Failed to parse valuesPerFile from \"" + filepath + "\": Invalid Integer\n");
        } catch (const std::out_of_range& err) {
            return fail("Failed to parse valuesPerFile from \"" + filepath + "\": Out of Integer range\n");
        }
    } else if (datasetType == "BLOCK") {
        config.dataset = std::make_unique<BlockDataset>(xVals, yVals, zVals, dimOrder, scheme);
    } else {
        return fail("Invalid dataset type \"" + datasetType + "\" in \"" + filepath + "\"\n");
    }

    return config;
}