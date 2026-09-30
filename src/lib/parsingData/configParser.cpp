#include "configParser.hpp"
#include "datasetConfig.hpp"
#include <string>
#include <fstream>
#include <iostream>
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

    if (!configFile.is_open()) {
        std::cerr << "Failed to open \"" << filepath << "\"" << std::endl;
        return config;
    }

    // read the name 
    std::string line;
    std::getline(configFile, line);
    if (line.substr(0,5) == "NAME:") {
        try {
            config.name = line.substr(5);
        } catch (const std::out_of_range& err) {
            std::cerr << "Failed to parse NAME in \"" << filepath << "\"" << std::endl;
            return config;
        }
    } else {
        std::cerr << "Failed to find NAME in \"" << filepath << "\"" << std::endl;
        return config;
    }

    // read the FILEPATH
    std::getline(configFile, line);
    if (line.substr(0,9) == "FILEPATH:") {
        try {
            config.filePath = line.substr(9);
        } catch (const std::out_of_range& err) {
            std::cerr << "Failed to parse FILEPATH in \"" << filepath << "\"" << std::endl;
            return config;
        }
    } else {
        std::cerr << "Failed to find FILEPATH in \"" << filepath << "\"" << std::endl;
        return config;
    }

    // read the xVals, yVals and zVals from the file, in the format "SPACIALDOMAIN:xvals:yvals:zvals"
    int xVals, yVals, zVals;
    std::getline(configFile, line);
    if (line.substr(0,14) == "SPACIALDOMAIN:") {
        std::string spacialDomain = line.substr(14);

        auto firstColon = spacialDomain.find(':');
        if (firstColon == std::string::npos) {
            std::cerr << "Spacial domain formatting error in \"" << filepath << "\"" << std::endl;
            return config;
        }
        std::string xValsStr = spacialDomain.substr(0, firstColon);
        spacialDomain = spacialDomain.substr(firstColon + 1);

        auto secondColon = spacialDomain.find(':');
        if (secondColon == std::string::npos) {
            std::cerr << "Spacial domain formatting error in \"" << filepath << "\"" << std::endl;
            return config;
        }
        std::string yValsStr = spacialDomain.substr(0, secondColon);
        std::string zValsStr = spacialDomain.substr(secondColon + 1);

        try {
            xVals = std::stoi(xValsStr);
            yVals = std::stoi(yValsStr);
            zVals = std::stoi(zValsStr);
        } catch (const std::invalid_argument& err) {
            std::cerr << "Failed to parse SPACIALDOMAIN values from \"" << filepath << "\": Invalid Integer" << std::endl;
            return config;
        } catch (const std::out_of_range& err) {
            std::cerr << "Failed to parse SPACIALDOMAIN values from  \"" << filepath << "\": Out of Integer range" << std::endl;
            return config;
        }
    } else {
        std::cerr << "Failed to find SPACIALDOMAIN in \"" << filepath << "\"" << std::endl;
        return config;
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
                std::cerr << "Error: DIMENSIONORDER option \"" << dimOrder << "\" is not supported" << std::endl;
                return config;
            }
        } catch (const std::out_of_range& err) {
            std::cerr << "Failed to parse DIMENSIONORDER in \"" << filepath << "\"" << std::endl;
            return config;
        }
    } else {
        std::cerr << "Failed to find DIMENSIONORDER in \"" << filepath << "\"" << std::endl;
        return config;
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
                std::cerr << "Error: INDEXSCHEME scheme \"" << indexScheme << "\" is not supported" << std::endl;
                return config;
            }
        } catch (const std::out_of_range& err) {
            std::cerr << "Failed to parse INDEXSCHEME in \"" << filepath << "\"" << std::endl;
            return config;
        }
    } else {
        std::cerr << "Failed to find INDEXSCHEME in \"" << filepath << "\"" << std::endl;
        return config;
    }
    
    // Optional fill value. Omitted datasets keep every sample.
    std::getline(configFile, line);
    if (line.substr(0, 7) == "NODATA:") {
        try {
            config.noData = std::stof(line.substr(7));
        } catch (const std::invalid_argument&) {
            std::cerr << "Failed to parse NODATA value from \"" << filepath << "\": Invalid float" << std::endl;
            return config;
        } catch (const std::out_of_range&) {
            std::cerr << "Failed to parse NODATA value from \"" << filepath << "\": Out of float range" << std::endl;
            return config;
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
            std::cerr << "DATASTRUCTURE not found in \"" << filepath << "\"" << std::endl;
            return config;
        }
    } else {
        std::cerr << "Failed to find DATASETSTRUCTURE in \"" << filepath << "\"" << std::endl;
        return config;
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
                        if (filename.empty() || name.empty() || attribute.empty()) {
                            std::cerr << "Attribute miss formatted in \"" << filepath << "\": " << line << std::endl;
                            return config;
                        }
                        try {
                            float lowerBound = std::stof(attribute.substr(0, attribute.find(":")));
                            float upperBound = std::stof(attribute.substr(attribute.find(":")+1));
                            dataset->addFile(filename, name, lowerBound, upperBound);
                        }catch (const std::invalid_argument& err) {
                            std::cerr << "Unable to parse bounds for attribute: " << attribute << std::endl;
                            return config;
                        } catch (const std::out_of_range& err) {
                            std::cerr << "Bounds out of float value range for attribute: " << attribute << std::endl;
                            return config;
                        }
                    } catch (const std::out_of_range& err) {
                        std::cerr << "Attribute miss formatted in \"" << filepath << "\": " << line << std::endl;
                        return config;
                    }
                } else {
                    std::cerr << "Failed to find ATTRIBUTE where expected in \"" << filepath << "\"" << std::endl;
                    return config;
                }
            }
        } catch (const std::invalid_argument& err) {
            std::cerr << "Failed to parse valuesPerFile from \"" << filepath << "\": Invalid Integer" << std::endl;
            return config;
        } catch (const std::out_of_range& err) {
            std::cerr << "Failed to parse valuesPerFile from \"" << filepath << "\": Out of Integer range" << std::endl;
            return config;
        }
    } else if (datasetType == "BLOCK") {
        config.dataset = std::make_unique<BlockDataset>(xVals, yVals, zVals, dimOrder, scheme);
    } else {
        std::cerr << "Invalid dataset type \"" << datasetType << "\" in \"" << filepath << "\"" << std::endl;
        return config;
    }
    
    config.parseError = false;  
    return config;
}