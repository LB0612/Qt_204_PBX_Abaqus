#include "SimulationArtifactStateService.h"

#include "ProjectInputHash.h"
#include "ProjectPaths.h"
#include "SimulationIntegrityService.h"

#include <QCryptographicHash>
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

QString SimulationArtifactStateService::fileSha256(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);

    constexpr qint64 kChunkSize = 1024 * 1024;

    while (!file.atEnd()) {
        const QByteArray chunk = file.read(kChunkSize);

        if (chunk.isEmpty()) {
            if (file.error() != QFile::NoError) {
                return QString();
            }
            break;
        }

        hash.addData(chunk);
    }

    return QString::fromLatin1(hash.result().toHex());
}

QString SimulationArtifactStateService::t2CompletionStampUtc(
    const QString &projectPath)
{
    const QFileInfo info(
        ProjectPaths::t2FinishedFlagPath(projectPath)
    );

    if (!info.exists() || !info.isFile()) {
        return QString();
    }

    return info.lastModified().toUTC().toString(Qt::ISODateWithMs);
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
               ProjectPaths::lastSuccessInputFingerprintPath(
                   projectPath
               ),
               current
           )
        || fingerprintFileMatches(
               ProjectPaths::runningInputFingerprintPath(
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
            ProjectPaths::t1FinishedFlagPath(projectPath))) {
        return false;
    }

    if (!isNonEmptyRegularFile(
            ProjectPaths::solverOdbPath(projectPath))) {
        return false;
    }

    if (QFile::exists(
            ProjectPaths::currentJobLockPath(projectPath))) {
        return false;
    }

    if (!solverFingerprintMatches(projectPath)) {
        return false;
    }

    const QString currentInput =
        ProjectInputHash::hashSolverInput(projectPath);

    QString integrityError;

    return SimulationIntegrityService::
        validateSolverResultIntegrity(
            projectPath,
            currentInput,
            integrityError,
            true
        );
}

ProjectInputHash::PostProcessManifest
SimulationArtifactStateService::postProcessManifest(
    const QString &projectPath)
{
    return ProjectInputHash::readPostProcessManifest(projectPath);
}

bool SimulationArtifactStateService::validateCompletePostProcess(
    const QString &projectPath,
    QString &errorMessage)
{
    if (!ProjectInputHash::validatePostProcessOutputs(
            projectPath,
            errorMessage)) {
        return false;
    }

    const QString currentPost =
        ProjectInputHash::hashPostProcessInput(projectPath);
    if (currentPost.isEmpty()) {
        errorMessage =
            QStringLiteral("无法计算后处理输入指纹。");
        return false;
    }

    return SimulationIntegrityService::
        validatePostProcessIntegrity(
            projectPath,
            currentPost,
            errorMessage,
            true
        );
}

bool SimulationArtifactStateService::hasCompletePostProcess(
    const QString &projectPath)
{
    if (!hasValidSolverResult(projectPath)) {
        return false;
    }

    if (!readSuccessFlag(
            ProjectPaths::t2FinishedFlagPath(projectPath))) {
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
    return validateCompletePostProcess(projectPath, outputError);
}
