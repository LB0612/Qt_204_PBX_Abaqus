#include "ProjectInputHash.h"

#include "ProjectPaths.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>

#include <cmath>

namespace ProjectInputHash {

namespace {

QString hashRelativeFiles(
    const QString &projectPath,
    const QStringList &relativePaths)
{
    if (projectPath.isEmpty()) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    const QDir projectDir(projectPath);

    for (const QString &relativePath : relativePaths) {
        QFile file(projectDir.filePath(relativePath));
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }

        hash.addData(relativePath.toUtf8());
        hash.addData(file.readAll());
    }

    return QString::fromLatin1(hash.result().toHex());
}

QString normalizedProjectPath(const QString &projectPath)
{
    return QDir::fromNativeSeparators(
        QDir::cleanPath(QFileInfo(projectPath).absoluteFilePath())
    );
}

bool projectPathsMatch(const QString &left, const QString &right)
{
#ifdef Q_OS_WIN
    return QString::compare(left, right, Qt::CaseInsensitive) == 0;
#else
    return left == right;
#endif
}

QString framePngPath(
    const QString &frameDir,
    const QString &prefix,
    int index)
{
    return QDir(frameDir).filePath(
        QStringLiteral("%1_%2.png")
            .arg(prefix)
            .arg(index, 8, 10, QLatin1Char('0'))
    );
}

bool isValidPngFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    if (file.size() < 64) {
        return false;
    }

    const QByteArray header = file.read(8);
    if (header
        != QByteArray::fromRawData(
            "\x89PNG\r\n\x1a\n",
            8)) {
        return false;
    }

    if (!file.seek(file.size() - 12)) {
        return false;
    }

    const QByteArray tail = file.read(12);
    return tail.contains("IEND");
}

} // namespace

QString hashConfigFiles(const QString &projectPath)
{
    return hashRelativeFiles(
        projectPath,
        ProjectPaths::parameterConfigRelativePaths()
    );
}

QString hashGeneratedFiles(const QString &projectPath)
{
    QStringList paths = ProjectPaths::parameterConfigRelativePaths();
    paths += ProjectPaths::generatedAbaqusRelativePaths();
    return hashRelativeFiles(projectPath, paths);
}

QString hashSolverInput(const QString &projectPath)
{
    if (projectPath.isEmpty()) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    const QDir projectDir(projectPath);

    QStringList paths = ProjectPaths::parameterConfigRelativePaths();
    paths += {
        ProjectPaths::t0ScriptRelativePath(),
        ProjectPaths::t1ScriptRelativePath(),
        ProjectPaths::userSubroutineRelativePath(),
    };

    for (const QString &relativePath : paths) {
        QFile file(projectDir.filePath(relativePath));
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }

        hash.addData(relativePath.toUtf8());
        hash.addData(file.readAll());
    }

    hash.addData(QByteArray("CALCULATION_PIPELINE_VERSION"));
    hash.addData(
        QByteArray::number(CALCULATION_PIPELINE_VERSION)
    );

    return QString::fromLatin1(hash.result().toHex());
}

QString hashPostProcessInput(const QString &projectPath)
{
    const QString solverSha = hashSolverInput(projectPath);
    if (solverSha.isEmpty() || projectPath.isEmpty()) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(solverSha.toLatin1());

    QFile t2File(ProjectPaths::t2ScriptPath(projectPath));
    if (!t2File.open(QIODevice::ReadOnly)) {
        return QString();
    }

    hash.addData(ProjectPaths::t2ScriptRelativePath().toUtf8());
    hash.addData(t2File.readAll());

    hash.addData(QByteArray("POSTPROCESS_PIPELINE_VERSION"));
    hash.addData(
        QByteArray::number(POSTPROCESS_PIPELINE_VERSION)
    );

    return QString::fromLatin1(hash.result().toHex());
}

GenerationManifest readGenerationManifest(const QString &projectPath)
{
    GenerationManifest manifest;

    QFile file(ProjectPaths::generationCompleteFlagPath(projectPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return manifest;
    }

    const QStringList lines =
        QString::fromUtf8(file.readAll())
            .split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        const int separator = line.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            continue;
        }

        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();

        if (key == QStringLiteral("version")) {
            manifest.version = value.toInt();
        } else if (key == QStringLiteral("status")) {
            manifest.status = value;
        } else if (key == QStringLiteral("config_sha256")) {
            manifest.configSha256 = value;
        } else if (key == QStringLiteral("generated_sha256")) {
            manifest.generatedSha256 = value;
        } else if (key == QStringLiteral("project_path")) {
            manifest.projectPath = value;
        }
    }

    const QString currentProjectPath = normalizedProjectPath(projectPath);

    manifest.valid =
        manifest.version == GENERATION_MANIFEST_VERSION
        && manifest.status == QStringLiteral("success")
        && !manifest.configSha256.isEmpty()
        && !manifest.generatedSha256.isEmpty()
        && !manifest.projectPath.isEmpty()
        && projectPathsMatch(
            normalizedProjectPath(manifest.projectPath),
            currentProjectPath
        );

    return manifest;
}

bool writeGenerationManifest(
    const QString &projectPath,
    const QString &configSha256,
    const QString &generatedSha256,
    QString &errorMessage)
{
    const QString normalizedPath = normalizedProjectPath(projectPath);
    const QString manifestText =
        QStringLiteral("version=%1\n"
                       "status=success\n"
                       "config_sha256=%2\n"
                       "generated_sha256=%3\n"
                       "project_path=%4\n")
            .arg(GENERATION_MANIFEST_VERSION)
            .arg(configSha256)
            .arg(generatedSha256)
            .arg(normalizedPath);

    QSaveFile flagFile(ProjectPaths::generationCompleteFlagPath(projectPath));
    if (!flagFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        errorMessage = QStringLiteral("无法写入 Abaqus 文件生成完成标志。");
        return false;
    }

    flagFile.write(manifestText.toUtf8());
    if (!flagFile.commit()) {
        errorMessage = QStringLiteral("Abaqus 文件生成完成标志保存失败。");
        return false;
    }

    return true;
}

PostProcessManifest readPostProcessManifest(const QString &projectPath)
{
    PostProcessManifest manifest;

    QFile file(ProjectPaths::postProcessManifestPath(projectPath));
    if (!file.open(QIODevice::ReadOnly)) {
        return manifest;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        return manifest;
    }

    const QJsonObject json = document.object();
    manifest.version = json.value(QStringLiteral("version")).toInt();
    manifest.postSha256 =
        json.value(QStringLiteral("postSha256")).toString();
    manifest.odbFrames =
        json.value(QStringLiteral("odbFrames")).toInt();
    manifest.curePngFrames =
        json.value(QStringLiteral("curePngFrames")).toInt();
    manifest.temperaturePngFrames =
        json.value(QStringLiteral("temperaturePngFrames")).toInt();
    manifest.stressPngFrames =
        json.value(QStringLiteral("stressPngFrames")).toInt();
    manifest.playbackFps =
        json.value(QStringLiteral("playbackFps")).toInt();

    const QJsonValue frameTimesValue =
        json.value(QStringLiteral("frameTimes"));

    if (!frameTimesValue.isArray()) {
        return manifest;
    }

    const QJsonArray frameTimesJson =
        frameTimesValue.toArray();
    manifest.frameTimes.clear();
    for (const QJsonValue &value : frameTimesJson) {
        if (!value.isDouble()) {
            return manifest;
        }

        manifest.frameTimes.append(value.toDouble());
    }

    const bool countsMatch =
        manifest.odbFrames > 0
        && manifest.odbFrames == manifest.curePngFrames
        && manifest.odbFrames == manifest.temperaturePngFrames
        && manifest.odbFrames == manifest.stressPngFrames;

    const bool frameTimesValid = [&manifest]() {
        if (manifest.frameTimes.size() != manifest.odbFrames) {
            return false;
        }

        for (int i = 0; i < manifest.frameTimes.size(); ++i) {
            const double value = manifest.frameTimes.at(i);

            if (!std::isfinite(value) || value < 0.0) {
                return false;
            }

            if (i > 0
                && value < manifest.frameTimes.at(i - 1)) {
                return false;
            }
        }

        return true;
    }();

    const bool playbackFpsValid =
        manifest.playbackFps > 0;

    manifest.valid =
        manifest.version == POSTPROCESS_MANIFEST_VERSION
        && !manifest.postSha256.isEmpty()
        && countsMatch
        && playbackFpsValid
        && frameTimesValid;

    return manifest;
}

bool validatePostProcessOutputs(
    const QString &projectPath,
    QString &errorMessage)
{
    const PostProcessManifest manifest =
        readPostProcessManifest(projectPath);
    if (!manifest.valid) {
        errorMessage = QStringLiteral("后处理清单无效。");
        return false;
    }

    const int expectedFrames = manifest.odbFrames;
    if (expectedFrames <= 0) {
        errorMessage = QStringLiteral("后处理清单帧数无效。");
        return false;
    }

    const QString resultsDir =
        ProjectPaths::resultsDirectoryPath(projectPath);
    const QString cureBase =
        QDir(resultsDir).filePath(QStringLiteral("guhuadu"));
    const QString tempBase =
        QDir(resultsDir).filePath(QStringLiteral("wendu"));
    const QString stressBase =
        QDir(resultsDir).filePath(QStringLiteral("yingli"));

    const struct {
        QString frameDir;
        QString prefix;
    } pngSets[] = {
        {cureBase + QStringLiteral("_frames"), QStringLiteral("Cure_SDV1_frame")},
        {tempBase + QStringLiteral("_frames"), QStringLiteral("NT11_frame")},
        {stressBase + QStringLiteral("_frames"), QStringLiteral("Stress_Mises_frame")},
    };

    for (const auto &pngSet : pngSets) {
        for (int index = 0; index < expectedFrames; ++index) {
            const QString pngPath =
                framePngPath(pngSet.frameDir, pngSet.prefix, index);
            if (!isValidPngFile(pngPath)) {
                errorMessage =
                    QStringLiteral("PNG 缺失或无效：%1").arg(pngPath);
                return false;
            }
        }
    }

    return true;
}

bool clearPostProcessOutputs(
    const QString &projectPath,
    QString &errorMessage)
{
    const QString resultsDir =
        ProjectPaths::resultsDirectoryPath(projectPath);
    const QStringList baseNames = {
        QStringLiteral("guhuadu"),
        QStringLiteral("wendu"),
        QStringLiteral("yingli"),
    };

    for (const QString &baseName : baseNames) {
        const QString frameDir =
            QDir(resultsDir).filePath(baseName + QStringLiteral("_frames"));

        if (QFileInfo::exists(frameDir)) {
            if (!QDir(frameDir).removeRecursively()) {
                errorMessage =
                    QStringLiteral("无法删除后处理帧目录：\n%1")
                        .arg(frameDir);
                return false;
            }
        }
    }

    const QString manifestPath =
        ProjectPaths::postProcessManifestPath(projectPath);
    if (QFileInfo::exists(manifestPath) && !QFile::remove(manifestPath)) {
        errorMessage =
            QStringLiteral("无法删除后处理清单：\n%1").arg(manifestPath);
        return false;
    }

    const QString integrityPath =
        ProjectPaths::postProcessIntegrityPath(projectPath);
    if (QFileInfo::exists(integrityPath) && !QFile::remove(integrityPath)) {
        errorMessage =
            QStringLiteral("无法删除后处理完整性记录：\n%1")
                .arg(integrityPath);
        return false;
    }

    const QString t2FlagPath =
        ProjectPaths::t2FinishedFlagPath(projectPath);
    if (QFileInfo::exists(t2FlagPath) && !QFile::remove(t2FlagPath)) {
        errorMessage =
            QStringLiteral("无法删除后处理完成标志：\n%1").arg(t2FlagPath);
        return false;
    }

    errorMessage.clear();
    return true;
}

} // namespace ProjectInputHash
