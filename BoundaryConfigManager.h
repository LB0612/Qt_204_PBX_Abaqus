#ifndef BOUNDARYCONFIGMANAGER_H
#define BOUNDARYCONFIGMANAGER_H

#include "BoundaryConfig.h"

#include <QString>

class BoundaryConfigManager
{
public:
    static bool save(
        const QString &projectPath,
        const BoundaryConfig &config
    );

    static bool load(
        const QString &projectPath,
        BoundaryConfig &config
    );

    static bool validate(
        const BoundaryConfig &config,
        QString &errorMessage
    );
};

#endif
