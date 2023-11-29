#pragma once

const float NO_DATA_VAL = 1.0000000e+35;

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
        {"QRAINf02.bin", {0.f, 0.01132}}, \
        {"QSNOWf02.bin", {0.f, 0.00135}}, \
        {"QVAPORf02.bin", {0.f, 0.02368}}, \
        {"TCf02.bin", {-83.00402, 31.51576}}, \
        {"Uf02.bin", {-79.47297, 85.17703}}, \
        {"Vf02.bin", {-76.03391, 82.95293}}, \
        {"Wf02.bin", {-9.06026, 28.61434}}, \
    } \
}
