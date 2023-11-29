#include "read_multifield.hpp"

int main(int argc, char **argv) {
    multiField isabel = readIsabel(IsabelTimestep02DirConfig);

    // check each attribute
    for (int attrIndex = 0; attrIndex < isabel.attributeDomain.size(); attrIndex++) {
        attribute &curr = isabel.attributeDomain[attrIndex];
        std::cout << "Checking values in attribute " << curr.name << " are in the valid range:" << std::endl;

        // check if any of the values are outside of the allowed range for this file
        std::vector<int> invalid_indexes = {};
        for (int valIndex = 0; valIndex < curr.values.size(); valIndex++) {
            if (
                curr.values[valIndex] != NO_DATA_VAL and (
                    curr.bounds.lower > curr.values[valIndex] or 
                    curr.values[valIndex] > curr.bounds.upper
                )) {
                    invalid_indexes.push_back(valIndex);                
            }
        }
        std::cout << "\tFound " << invalid_indexes.size() << " points outside of the allowed range" << std::endl;
    }
    return 0;
}