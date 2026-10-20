#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "../lib/parsingData/datasetReader.hpp"
#include "../lib/multiField/multiField.hpp"
#include "../lib/parsingData/datasetReader.hpp"



int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config.txt>\n";
        return 2;
    }
    MultiField isabel = readDataset(argv[1]);

    if (!isabel.ok()) {
        std::cerr << isabel.error;
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
        if (!invalid_indexes.empty()) {
            const std::size_t exampleCount = std::min<std::size_t>(2, invalid_indexes.size());
            std::cout << "\tExample Values:";
            for (std::size_t i = 0; i < exampleCount; ++i) {
                if (i != 0) std::cout << ",";
                std::cout << " " << curr.values[invalid_indexes[i]];
            }
            std::cout << std::endl;
        }
    }
    return 0;
}