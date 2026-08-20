#ifndef MOLDCONFIG_H
#define MOLDCONFIG_H

struct MoldConfig
{
    double density = 7.85e-09;
    double elasticModulus = 210000.0;
    double poissonRatio = 0.3;
    double thermalConductivity = 45.0;
    double specificHeat = 480000000.0;

    int schemaVersion = 1;
};

#endif
