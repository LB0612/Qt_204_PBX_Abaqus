#ifndef PROJECTINPUTHASH_H
#define PROJECTINPUTHASH_H

#include <QString>
#include <QVector>

namespace ProjectInputHash {

constexpr int GENERATION_MANIFEST_VERSION = 7;
constexpr int CALCULATION_PIPELINE_VERSION = 2;
constexpr int POSTPROCESS_PIPELINE_VERSION = 3;

QString hashConfigFiles(const QString &projectPath);
QString hashGeneratedFiles(const QString &projectPath);
QString hashSolverInput(const QString &projectPath);
QString hashPostProcessInput(const QString &projectPath);

struct GenerationManifest
{
    int version = 0;
    QString status;
    QString configSha256;
    QString generatedSha256;
    QString projectPath;
    bool valid = false;
};

struct PostProcessManifest
{
    int version = 0;
    QString postSha256;
    int odbFrames = 0;
    int curePngFrames = 0;
    int temperaturePngFrames = 0;
    int stressPngFrames = 0;
    int cureVideoFrames = 0;
    int temperatureVideoFrames = 0;
    int stressVideoFrames = 0;
    qint64 cureVideoBytes = 0;
    qint64 temperatureVideoBytes = 0;
    qint64 stressVideoBytes = 0;
    int videoFps = 0;
    QVector<double> frameTimes;
    bool valid = false;
};

GenerationManifest readGenerationManifest(const QString &projectPath);
bool writeGenerationManifest(
    const QString &projectPath,
    const QString &configSha256,
    const QString &generatedSha256,
    QString &errorMessage
);

PostProcessManifest readPostProcessManifest(const QString &projectPath);

bool isValidPngFile(const QString &path);
bool validatePostProcessOutputs(const QString &projectPath, QString &errorMessage);

QString resultsDirectory(const QString &projectPath);
QString runningInputFingerprintPath(const QString &projectPath);
QString lastSuccessInputFingerprintPath(const QString &projectPath);
QString runningPostFingerprintPath(const QString &projectPath);
QString lastSuccessPostFingerprintPath(const QString &projectPath);

QString t1FinishedFlagPath(const QString &projectPath);
QString t2FinishedFlagPath(const QString &projectPath);
QString solverOdbPath(const QString &projectPath);

QString currentJobName(const QString &projectPath);
QString currentJobLockPath(const QString &projectPath);

} // namespace ProjectInputHash

#endif
