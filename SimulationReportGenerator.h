#ifndef SIMULATIONREPORTGENERATOR_H
#define SIMULATIONREPORTGENERATOR_H

#include <QString>

class SimulationReportGenerator
{
public:
    static bool generate(
        const QString &projectPath,
        QString &errorMessage);
};

#endif
