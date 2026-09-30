#pragma once 

#include "../multiField/multiField.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <memory>
#include <optional>

enum DatasetFormat {BLOCK, ATTRIBUTEPERFILE};
enum IndexScheme {ROWMAJOR, COLUMNMAJOR};
enum DimensionOrder {WIDTH_HEIGHT_DEPTH, WIDTH_DEPTH_HEIGHT};

inline std::string indexSchemeToString(IndexScheme scheme) {
    switch (scheme) {
        case ROWMAJOR: return "ROWMAJOR";
        case COLUMNMAJOR: return "COLUMNMAJOR";
    }
    return "";
}

inline std::string dimensionOrderToString(DimensionOrder order) {
    switch (order) {
        case WIDTH_HEIGHT_DEPTH: return "WIDTH_HEIGHT_DEPTH";
        case WIDTH_DEPTH_HEIGHT: return "WIDTH_DEPTH_HEIGHT";
    }
    return "";
}

class Dataset {
public:
    DatasetFormat format;
    int xVals, yVals, zVals;
    DimensionOrder dimOrder;
    IndexScheme scheme;
    // Constructor to initialize format
    Dataset(DatasetFormat fmt, int xVals, int yVals, int zVals, DimensionOrder order, IndexScheme scheme) : format(fmt), xVals(xVals), yVals(yVals), zVals(zVals), scheme(scheme), dimOrder(order) {}
    virtual ~Dataset() = default;
    void virtual printDataset() const = 0;
    std::function<int(int, int, int, int, int, int)> getIndexFunction() const {
        switch (scheme) {
            case ROWMAJOR:
                switch (dimOrder) {
                    case WIDTH_HEIGHT_DEPTH:
                        return [this](int x, int y, int z, int xVals, int yVals, int zVals) { return x + (y * xVals) + (z * xVals * yVals); };
                    case WIDTH_DEPTH_HEIGHT:
                        return [this](int x, int y, int z, int xVals, int yVals, int zVals) { return x + (z * xVals) + (y * xVals * zVals); };
                    default:
                        throw std::invalid_argument("Unknown dimension order");
                }
            case COLUMNMAJOR:
                switch (dimOrder) {
                    case WIDTH_HEIGHT_DEPTH:
                        return [this](int x, int y, int z, int xVals, int yVals, int zVals) { return z + zVals * (y + yVals * x); };
                    case WIDTH_DEPTH_HEIGHT:
                        return [this](int x, int y, int z, int xVals, int yVals, int zVals) { return y + yVals * (z + zVals * x); };
                    default:
                        throw std::invalid_argument("Unknown dimension order");
                }
            default:
                throw std::invalid_argument("Unknown index scheme");
        }
    }
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
    AttributePerFileDataset(int valuesPerFile, int xVals, int yVals, int zVals, DimensionOrder order, IndexScheme scheme) : Dataset(ATTRIBUTEPERFILE, xVals, yVals, zVals, order, scheme), valuesPerFile(valuesPerFile) {}
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
        std::cout << "demsionOrder: " << dimensionOrderToString(dimOrder) << std::endl;
        std::cout << "indexingScheme: " << indexSchemeToString(scheme) << std::endl;
        std::cout << "valuesPerFile: " << valuesPerFile << std::endl;
        for (auto file : files) {
            std::cout << "filename: " << file.filename << "; name: " << file.name << "; lowerBound: " << file.lowerBound << "; upperBound: " << file.upperBound << std::endl;
        }
    }
};

class BlockDataset : public Dataset {
private:
    struct BlockAttribute {
        std::string name;
        int start;
        int stride;
        float lowerBound;
        float upperBound; 
    };
    std::vector<BlockAttribute> attributes;
public:
    BlockDataset(int xVals, int yVals, int zVals, DimensionOrder order, IndexScheme scheme) : Dataset(BLOCK, xVals, yVals, zVals, order, scheme) {}
    void printDataset() const {
        std::cout << "BlockDataset" << std::endl;
        std::cout << "demsionOrder: " << dimensionOrderToString(dimOrder) << std::endl;
        std::cout << "indexingScheme: " << indexSchemeToString(scheme) << std::endl;
        std::cout << "xVals: " << xVals << "; yVals: " << yVals << "; zVals: " << zVals << std::endl;
    }
};

struct DatasetDirConfig {
    bool parseError = true;
    std::string filePath;
    std::string name;
    // Absent means the dataset has no fill value. Present means that exact
    // float is missing data and becomes NaN when the attribute files are read.
    std::optional<float> noData;
    std::unique_ptr<Dataset> dataset;

    void printConfig() const {
        std::cout << "DatasetDirConfig" << std::endl;
        std::cout << "filePath: " << filePath << "; name: " << name << std::endl;
        if (noData) std::cout << "noData: " << *noData << std::endl;
        if (dataset) dataset->printDataset();
        else std::cout << "Dataset obj is null" << std::endl;
    }
};





