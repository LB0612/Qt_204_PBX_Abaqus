#ifndef ABAQUSFILEGENERATOR_H
#define ABAQUSFILEGENERATOR_H

#include "BoundaryConfig.h"
#include "ExplosiveConfig.h"
#include "MoldConfig.h"
#include "StructureConfig.h"

#include <QString>

class AbaqusFileGenerator
{
public:
    static bool generate(
        const QString &projectPath,
        const StructureConfig &structure,
        const ExplosiveConfig &explosive,
        const MoldConfig &mold,
        const BoundaryConfig &boundary,
        QString &errorMessage
    );

private:
    static bool loadTemplate(
        const QString &resourcePath,
        QString &content,
        QString &errorMessage
    );

    static bool saveFile(
        const QString &filePath,
        const QString &content,
        QString &errorMessage
    );

    static QString number(double value);
};

#endif
