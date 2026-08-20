#ifndef STRUCTURECONFIGMANAGER_H
#define STRUCTURECONFIGMANAGER_H

#include "StructureConfig.h"

#include <QString>

class StructureConfigManager
{
public:
    static bool save(
        const QString &projectPath,
        const StructureConfig &config
    );

    static bool load(
        const QString &projectPath,
        StructureConfig &config
    );

    static bool validate(
        const StructureConfig &config,
        QString &errorMessage
    );
};

#endif
