#ifndef MOLDCONFIGMANAGER_H
#define MOLDCONFIGMANAGER_H

#include "MoldConfig.h"

#include <QString>

class MoldConfigManager
{
public:
    static bool save(
        const QString &projectPath,
        const MoldConfig &config
    );

    static bool load(
        const QString &projectPath,
        MoldConfig &config
    );

    static bool validate(
        const MoldConfig &config,
        QString &errorMessage
    );
};

#endif
