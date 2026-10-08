# TraitVis

Final year undergraduate project by Micheal Nestor. A tool for generating feature level-sets in arbitrary multifield datasets. Please read report.pdf for an indepth explainaition of feature level-sets and how this tool works. 

## Build Instructions 

Premake is used for building:  

Generate make files:  
```
$ premake5 gmake
```

Make binaries and object files:
```
$ make config=release
```

Binaries will be available in ./bin/Release/  

## Loading Datasets

To load a datset,you will need to describe it using our configuration file format. Currently TraitVis only supports mutlfield datasets where attributes are defined in separate binary files of the same size in the brick of floats format. The Hurricane Isabel datset is an example of a dataset which TratVis currently supports and can be donwloaded [here](https://www.earthsystemgrid.org/dataset/isabeldata.html).

The configuration file schema is shown here with an example for timestep24 of Isabel below it. `NODATA` is optional. When it is present, that exact float value is treated as missing data.

![Configuration file schema](https://imgur.com/VZcXeel.png)

```
NAME:isabell-timestep-24
FILEPATH:./
SPATIALDOMAIN:500:100:500
DIMENSIONORDER:WIDTH_DEPTH_HEIGHT
INDEXSCHEME:ROWMAJOR
NODATA:1e35
DATASETSTRUCTURE:ATTRIBUTEPERFILE:25000000
ATTRIBUTE:CLOUDf24.bin:Total cloud:0:0.0032
ATTRIBUTE:Pf24.bin:Pressure:-5471.85791:3225.42578
ATTRIBUTE:PRECIPf24.bin:Precipitation:0:0.01672
ATTRIBUTE:QCLOUDf24.bin:Cloud moisture mixing ratio:0:0.00332
ATTRIBUTE:QGRAUPf24.bin:Graupel mixing ratio:0:0.01638
ATTRIBUTE:QICEf24.bin:Cloud ice mixing ratio:0:0.00099
ATTRIBUTE:QRAINf24.bin:Rain mixing ratio:0:0.01132
ATTRIBUTE:QSNOWf24.bin:Snow mixing ratio:0:0.00135
ATTRIBUTE:QVAPORf24.bin:Water vapor mixing ratio:0:0.02368
ATTRIBUTE:TCf24.bin:Temperature (Celsius):-83.00402:31.51576
ATTRIBUTE:Uf24.bin:X wind speed:-79.47297:85.17703
ATTRIBUTE:Vf24.bin:Y wind speed:-76.03391:82.95293
ATTRIBUTE:Wf24.bin:Z wind speed:-9.06026:28.61434
ATTRIBUTE:velocity.bin:Wind Velocity:0:85.17703
```

## Generating Feature Level-Sets

To generate a feature level-set, first define a trait by selecting points in attribute space. Then select a Euclidean distance, select a colour, give the feature level-set a name, and click the generate level-set button. A surface for your feature level-set will then appear. The console will output the number of vertices, contianed in the surface, if this is 0 then your no points in the spatial domain map to your trait in attribute space. 
