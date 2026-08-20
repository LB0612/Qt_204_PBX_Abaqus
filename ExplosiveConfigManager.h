#ifndef EXPLOSIVECONFIGMANAGER_H
#define EXPLOSIVECONFIGMANAGER_H

#include "ExplosiveConfig.h"

#include <QString>

class ExplosiveConfigManager
{
public:
    static bool save(
        const QString &projectPath,
        const ExplosiveConfig &config
    );

    static bool load(
        const QString &projectPath,
        ExplosiveConfig &config
    );

    static bool validate(
        const ExplosiveConfig &config,
        QString &errorMessage
    );
};

#endif
