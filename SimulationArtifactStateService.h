#ifndef SIMULATIONARTIFACTSTATESERVICE_H
#define SIMULATIONARTIFACTSTATESERVICE_H

#include "ProjectInputHash.h"

#include <QString>

class SimulationArtifactStateService
{
public:
    static bool readSuccessFlag(const QString &flagPath);
    static bool isNonEmptyRegularFile(const QString &path);

    static bool fingerprintFileMatches(
        const QString &storedPath,
        const QString &expectedFingerprint
    );

    static bool solverFingerprintMatches(const QString &projectPath);
    static bool hasValidSolverResult(const QString &projectPath);
    static bool hasCompletePostProcess(const QString &projectPath);

    static ProjectInputHash::PostProcessManifest postProcessManifest(
        const QString &projectPath
    );
};

#endif
