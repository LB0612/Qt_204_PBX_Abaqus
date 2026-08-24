#ifndef PROJECTINPUTHASH_H
#define PROJECTINPUTHASH_H

#include <QString>

namespace ProjectInputHash {

constexpr int GENERATION_MANIFEST_VERSION = 1;
constexpr int CALCULATION_PIPELINE_VERSION = 1;

QString hashConfigFiles(const QString &projectPath);
QString hashGeneratedFiles(const QString &projectPath);
QString hashSolverInput(const QString &projectPath);

struct GenerationManifest
{
    int version = 0;
    QString status;
    QString configSha256;
    QString generatedSha256;
    bool valid = false;
};

GenerationManifest readGenerationManifest(const QString &projectPath);
bool writeGenerationManifest(
    const QString &projectPath,
    const QString &configSha256,
    const QString &generatedSha256,
    QString &errorMessage
);

QString currentJobName(const QString &projectPath);
QString currentJobLockPath(const QString &projectPath);

} // namespace ProjectInputHash

#endif
