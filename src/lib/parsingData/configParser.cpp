#include "configParser.hpp"
#include "datasetConfig.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>

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
        try {
            std::string spacialDomain = line.substr(14);
            std::string xValsStr = spacialDomain.substr(0, spacialDomain.find(":"));
            spacialDomain = spacialDomain.substr(spacialDomain.find(":")+1);
            std::string yValsStr = spacialDomain.substr(0, spacialDomain.find(":"));
            std::string zValsStr = spacialDomain.substr(spacialDomain.find(":")+1);
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
        } catch (const std::out_of_range& err) {
            std::cerr << "Spacial domain formatting error in \"" << filepath << "\"" << std::endl;
        }


    } else {
        std::cerr << "Failed to find SPACIALDOMAIN in \"" << filepath << "\"" << std::endl;
        return config;
    }
    
    
    // read the datasetStructure
    std::string datasetType;
    std::string datasetStructure;
    std::getline(configFile, line);
    if (line.substr(0,17) == "DATASETSTRUCTURE:") {
        try {
            datasetStructure = line.substr(17);
            datasetType = datasetStructure.substr(0, datasetStructure.find(":"));
        } catch (const std::out_of_range& err) {
            std::cerr << "DATASTRUCTURE not found in \"" << filepath << "\"" << std::endl;
            return config;
        }
    } else {
        std::cout << line << std::endl;
        std::cerr << "Failed to parse DATASETSTRUCTURE from \"" << filepath << "\"" << std::endl;
        return config;
    }

    if (datasetType == "ATTRIBUTEPERFILE") {
        try {
            int valuesPerFile = std::stoi(datasetStructure.substr(datasetStructure.find(":")+1));
            config.dataset = new AttributePerFileDataset(valuesPerFile, xVals, yVals, zVals);

            // attributes will be listed as such in the file: ATTRIBUTE:filename:attributename:minfloatval:maxfloatval
            while (std::getline(configFile, line)) {
                if (line.substr(0,10) == "ATTRIBUTE:") {
                    try {
                        std::string attribute = line.substr(10);
                        std::string filename = attribute.substr(0, attribute.find(":"));
                        attribute = attribute.substr(attribute.find(":")+1);
                        std::string name = attribute.substr(0, attribute.find(":"));
                        attribute = attribute.substr(attribute.find(":")+1);
                        try {
                            float lowerBound = std::stof(attribute.substr(0, attribute.find(":")));
                            float upperBound = std::stof(attribute.substr(attribute.find(":")+1));
                            static_cast<AttributePerFileDataset*>(config.dataset)->addFile(filename, name, lowerBound, upperBound);
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
        config.dataset = new BlockDataset(xVals, yVals, zVals);
    } else {
        std::cerr << "Invalid dataset type \"" << datasetType << "\" in \"" << filepath << "\"" << std::endl;
        return config;
    }
    
    config.parseError = false;  
    return config;
}