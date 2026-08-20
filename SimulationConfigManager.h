#ifndef SIMULATIONCONFIGMANAGER_H
#define SIMULATIONCONFIGMANAGER_H

#include "SimulationConfig.h"

#include <QString>

class SimulationConfigManager
{
public:
    static bool save(
        const QString &projectPath,
        const SimulationConfig &config
    );

    static bool load(
        const QString &projectPath,
        SimulationConfig &config
    );

    static bool validate(
        const SimulationConfig &config,
        QString &errorMessage
    );
};

#endif
