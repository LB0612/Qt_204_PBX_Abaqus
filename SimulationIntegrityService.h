#ifndef SIMULATIONINTEGRITYSERVICE_H
#define SIMULATIONINTEGRITYSERVICE_H

#include <QString>

class SimulationIntegrityService
{
public:
    static bool writeSolverResultIntegrity(
        const QString &projectPath,
        const QString &inputSha256,
        QString &errorMessage
    );

    static bool validateSolverResultIntegrity(
        const QString &projectPath,
        const QString &inputSha256,
        QString &errorMessage,
        bool allowBootstrap = true
    );

    static bool writePostProcessIntegrity(
        const QString &projectPath,
        const QString &postSha256,
        QString &errorMessage
    );

    static bool validatePostProcessIntegrity(
        const QString &projectPath,
        const QString &postSha256,
        QString &errorMessage,
        bool allowBootstrap = true
    );
};

#endif
