#ifndef PROJECTPARAMETERSERVICE_H
#define PROJECTPARAMETERSERVICE_H

#include "BoundaryConfig.h"
#include "ExplosiveConfig.h"
#include "MoldConfig.h"
#include "SimulationConfig.h"
#include "StructureConfig.h"

#include <QString>

struct ProjectParameters
{
    StructureConfig structure;
    ExplosiveConfig explosive;
    MoldConfig mold;
    BoundaryConfig boundary;
    SimulationConfig simulation;
};

class ProjectParameterService
{
public:
    static QString structureConfigPath(const QString &projectPath);
    static QString explosiveConfigPath(const QString &projectPath);
    static QString moldConfigPath(const QString &projectPath);
    static QString boundaryConfigPath(const QString &projectPath);
    static QString simulationConfigPath(const QString &projectPath);

    // All five configs must load successfully.
    static bool loadAll(
        const QString &projectPath,
        ProjectParameters &parameters,
        QString &errorMessage
    );

    // Missing files keep defaults; existing but invalid files fail.
    static bool loadAvailable(
        const QString &projectPath,
        ProjectParameters &parameters,
        QString &errorMessage
    );
};

#endif
