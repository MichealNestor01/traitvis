#include "read_multifield.hpp"

#define IsabelTimestep02DirConfig multiFieldDirConfig{ \
    /* valuesPerFile */ 25000000, \
    /* directory path */ "/home/michealnestor/University/final-project/dataset/timestep02/", \
    /* files */ { \
        {"CLOUDf02.bin", {0.f, 0.00332}}, \
        {"Pf02.bin", {-5471.85791, 3225.42578}}, \
        {"PRECIPf02.bin", {0.f, 0.01672}}, \
        {"QCLOUDf02.bin", {0.f, 0.00332}}, \
        {"QGRAUPf02.bin", {0.f, 0.01638}}, \
        {"QICEf02.bin", {0.f, 0.00099}}, \
        {"QRAINf02.bin", {0.f, 0.00332}}, /*THIS RANGE IS MISSING ON THE WEBSITE*/ \
        {"QSNOWf02.bin", {0.f, 0.00135}}, \
        {"QVAPORf02.bin", {0.f, 0.02368}}, \
        {"TCf02.bin", {-83.00402, 31.51576}}, \
        {"Uf02.bin", {-79.47297, 85.17703}}, \
        {"Vf02.bin", {-76.03391, 82.95293}}, \
        {"Wf02.bin", {-9.06026, 28.61434}}, \
    } \
}
#define NO_DATA_VAL 1.0000000e+35

int main(int argc, char **argv) {
    // read in the files, and check contents
    for (int i = 0; i < IsabelTimestep02DirConfig.files.size(); i++) {
        std::cout << "Checking " << IsabelTimestep02DirConfig.files[i].filename << std::endl;

        // create the filepath of the current file
        std::string path =  IsabelTimestep02DirConfig.dirPath + IsabelTimestep02DirConfig.files[i].filename;

        // get the values from the file
        std::vector<float> vals = readFloatBinaryFile(path, IsabelTimestep02DirConfig.valuesPerFile);

        std::cout << "Read values, now checking all values fall in range" << std::endl;

        // check if any of the values are outside of the allowed range for this file
        std::vector<int> invalid_indexes = {};
        for (int j = 0; j < vals.size(); j++) {
            
            if (
                vals[i] != NO_DATA_VAL and (
                    IsabelTimestep02DirConfig.files[i].bounds.lower > vals[j] or 
                    vals[j] > IsabelTimestep02DirConfig.files[i].bounds.upper
                )) {
                    invalid_indexes.push_back(j);                
            }
        }
        std::cout << "Found " << invalid_indexes.size() << " points outside of the allowed range" << std::endl;
    }
    return 0;
}