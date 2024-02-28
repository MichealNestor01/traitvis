#pragma once 

#include "../multiField/multiField.hpp"
#include <string>
#include <vector>
#include <iostream>

enum DatasetFormat {BLOCK, ATTRIBUTEPERFILE};

class Dataset {
public:
    DatasetFormat format;
    int xVals, yVals, zVals;
    // Constructor to initialize format
    Dataset(DatasetFormat fmt, int xVals, int yVals, int zVals) : format(fmt), xVals(xVals), yVals(yVals), zVals(zVals) {}
    ~Dataset() {}
    void virtual printDataset() const = 0;
};

class AttributePerFileDataset : public Dataset {
private:
    struct AttributeFile {
        std::string filename;
        std::string name;
        float lowerBound;
        float upperBound; 
    };
    int valuesPerFile;
    std::vector<AttributeFile> files; 

public:
    AttributePerFileDataset(int valuesPerFile, int xVals, int yVals, int zVals) : Dataset(ATTRIBUTEPERFILE, xVals, yVals, zVals), valuesPerFile(valuesPerFile) {}
    void addFile(std::string filename, std::string name, float lowerBound, float upperBound) {
        AttributeFile file;
        file.filename = filename;
        file.name = name;
        file.lowerBound = lowerBound;
        file.upperBound = upperBound;
        files.push_back(file);
    }

    const std::vector<AttributeFile>& getFiles() const {
        return files;
    }

    int getValuesPerFile() const {
        return valuesPerFile;
    }

    void printDataset() const {
        std::cout << "AttributePerFileDataset" << std::endl;
        std::cout << "xVals: " << xVals << "; yVals: " << yVals << "; zVals: " << zVals << std::endl;
        std::cout << "valuesPerFile: " << valuesPerFile << std::endl;
        for (auto file : files) {
            std::cout << "filename: " << file.filename << "; name: " << file.name << "; lowerBound: " << file.lowerBound << "; upperBound: " << file.upperBound << std::endl;
        }
    }
};

class BlockDataset : public Dataset {
private:
    struct Attribute {
        int start;
        int stride;
        float lowerBound;
        float upperBound; 
    };
public:
    BlockDataset(int xVals, int yVals, int zVals) : Dataset(BLOCK, xVals, yVals, zVals) {}
    void printDataset() const {
        std::cout << "BlockDataset" << std::endl;
        std::cout << "xVals: " << xVals << "; yVals: " << yVals << "; zVals: " << zVals << std::endl;
    }
};

struct DatasetDirConfig {
    bool parseError = true;
    std::string filePath;
    std::string name;
    Dataset *dataset = nullptr;

    ~DatasetDirConfig() {
        if (dataset != nullptr) delete dataset;
    }

    void printConfig() const {
        std::cout << "DatasetDirConfig" << std::endl;
        std::cout << "filePath: " << filePath << "; name: " << name << std::endl;
        dataset->printDataset();
    }
};





