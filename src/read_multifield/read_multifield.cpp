#include <cmath>
#include <iostream>
#include <vector>

#include "../lib/parsingData/datasetReader.hpp"
#include "../lib/multiField/multiField.hpp"
#include "../lib/parsingData/datasetReader.hpp"



int main(int argc, char **argv) {
    MultiField isabel = readDataset("/home/michealnestor/University/final-project/dataset/timestep02/config.txt");

    if (isabel.readError) {
        std::cerr << "Error reading dataset" << std::endl;
        return 1;
    }

    // check each attribute
    for (int attrIndex = 0; attrIndex < isabel.attributeDomain.size(); attrIndex++) {
        Attribute &curr = isabel.attributeDomain[attrIndex];
        std::cout << "Checking values in attribute \"" << curr.name << "\" are in the valid range: " \
            << curr.bounds.lower << ":" << curr.bounds.upper << std::endl;

        // check if any of the values are outside of the allowed range for this file
        std::vector<int> invalid_indexes = {};
        for (int valIndex = 0; valIndex < curr.values.size(); ++valIndex) {
            if (std::isnan(curr.values[valIndex])) continue;
            if (curr.bounds.lower > curr.values[valIndex] or curr.values[valIndex] > curr.bounds.upper) {
                invalid_indexes.push_back(valIndex);
            }
        }
        std::cout << "\tFound " << invalid_indexes.size() << " points outside of the allowed range" << std::endl;
        if (invalid_indexes.size() > 0) {
            std::cout << "\tExample Values: " << curr.values[invalid_indexes[0]] << ", " << curr.values[invalid_indexes[1]] << std::endl; 
        }
    }
    return 0;
}