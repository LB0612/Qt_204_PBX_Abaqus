#ifndef EXPLOSIVECONFIG_H
#define EXPLOSIVECONFIG_H

struct ExplosiveConfig
{
    double density = 1.68e-09;

    double initialElasticModulus = 10.0;
    double initialPoissonRatio = 0.3;

    double finalElasticModulus = 8670.0;
    double finalPoissonRatio = 0.3;

    double thermalConductivity = 0.495;
    double yieldStress = 60.0;
    double specificHeat = 1330000000.0;
    double expansionCoefficient = 1.21e-05;
};

#endif
