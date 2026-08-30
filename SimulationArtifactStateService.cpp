#include "SimulationArtifactStateService.h"

#include <QFile>
#include <QFileInfo>

bool SimulationArtifactStateService::readSuccessFlag(
    const QString &flagPath)
{
    QFile flagFile(flagPath);
    if (!flagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QString content =
        QString::fromUtf8(flagFile.readAll()).trimmed();
    return content == QStringLiteral("success");
}

bool SimulationArtifactStateService::isNonEmptyRegularFile(
    const QString &path)
{
    const QFileInfo info(path);
    return info.exists() && info.isFile() && info.size() > 0;
}

bool SimulationArtifactStateService::fingerprintFileMatches(
    const QString &storedPath,
    const QString &expectedFingerprint)
{
    if (storedPath.isEmpty() || expectedFingerprint.isEmpty()) {
        return false;
    }

    QFile file(storedPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QString stored =
        QString::fromLatin1(file.readAll()).trimmed();

    return !stored.isEmpty() && stored == expectedFingerprint;
}

bool SimulationArtifactStateService::solverFingerprintMatches(
    const QString &projectPath)
{
    const QString current =
        ProjectInputHash::hashSolverInput(projectPath);

    return fingerprintFileMatches(
               ProjectInputHash::lastSuccessInputFingerprintPath(
                   projectPath
               ),
               current
           )
        || fingerprintFileMatches(
               ProjectInputHash::runningInputFingerprintPath(
                   projectPath
               ),
               current
           );
}

bool SimulationArtifactStateService::hasValidSolverResult(
    const QString &projectPath)
{
    if (projectPath.isEmpty()) {
        return false;
    }

    if (!readSuccessFlag(
            ProjectInputHash::t1FinishedFlagPath(projectPath))) {
        return false;
    }

    if (!isNonEmptyRegularFile(
            ProjectInputHash::solverOdbPath(projectPath))) {
        return false;
    }

    if (QFile::exists(
            ProjectInputHash::currentJobLockPath(projectPath))) {
        return false;
    }

    return solverFingerprintMatches(projectPath);
}

ProjectInputHash::PostProcessManifest
SimulationArtifactStateService::postProcessManifest(
    const QString &projectPath)
{
    return ProjectInputHash::readPostProcessManifest(projectPath);
}

bool SimulationArtifactStateService::hasCompletePostProcess(
    const QString &projectPath)
{
    if (!hasValidSolverResult(projectPath)) {
        return false;
    }

    if (!readSuccessFlag(
            ProjectInputHash::t2FinishedFlagPath(projectPath))) {
        return false;
    }

    const ProjectInputHash::PostProcessManifest manifest =
        postProcessManifest(projectPath);
    if (!manifest.valid) {
        return false;
    }

    const QString currentPost =
        ProjectInputHash::hashPostProcessInput(projectPath);
    if (currentPost.isEmpty()
        || currentPost != manifest.postSha256) {
        return false;
    }

    QString outputError;
    if (!ProjectInputHash::validatePostProcessOutputs(
            projectPath,
            outputError)) {
        return false;
    }

    return true;
}
