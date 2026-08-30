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
    manifest.cureVideoFrames =
        json.value(QStringLiteral("cureVideoFrames")).toInt();
    manifest.temperatureVideoFrames =
        json.value(QStringLiteral("temperatureVideoFrames")).toInt();
    manifest.stressVideoFrames =
        json.value(QStringLiteral("stressVideoFrames")).toInt();
    manifest.cureVideoBytes =
        json.value(QStringLiteral("cureVideoBytes")).toVariant().toLongLong();
    manifest.temperatureVideoBytes =
        json.value(QStringLiteral("temperatureVideoBytes"))
            .toVariant()
            .toLongLong();
    manifest.stressVideoBytes =
        json.value(QStringLiteral("stressVideoBytes"))
            .toVariant()
            .toLongLong();
    manifest.videoFps =
        json.value(QStringLiteral("videoFps")).toInt();

    const QJsonArray frameTimesJson =
        json.value(QStringLiteral("frameTimes")).toArray();
    manifest.frameTimes.clear();
    for (const QJsonValue &value : frameTimesJson) {
        manifest.frameTimes.append(value.toDouble());
    }

    const bool countsMatch =
        manifest.odbFrames > 0
        && manifest.odbFrames == manifest.curePngFrames
        && manifest.odbFrames == manifest.temperaturePngFrames
        && manifest.odbFrames == manifest.stressPngFrames
        && manifest.odbFrames == manifest.cureVideoFrames
        && manifest.odbFrames == manifest.temperatureVideoFrames
        && manifest.odbFrames == manifest.stressVideoFrames;

    const bool bytesRecorded =
        manifest.cureVideoBytes > 0
        && manifest.temperatureVideoBytes > 0
        && manifest.stressVideoBytes > 0;

    const bool frameTimesMatch =
        manifest.frameTimes.size() == manifest.odbFrames;

    manifest.valid =
        manifest.version == 3
        && !manifest.postSha256.isEmpty()
        && countsMatch
        && bytesRecorded
        && frameTimesMatch;

    return manifest;
}

QString resultsDirectory(const QString &projectPath)
{
    return ProjectPaths::resultsDirectoryPath(projectPath);
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

    const QString resultsDir = resultsDirectory(projectPath);
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

    const struct {
        QString aviPath;
        qint64 expectedBytes;
        int expectedVideoFrames;
    } aviSets[] = {
        {
            cureBase + QStringLiteral(".avi"),
            manifest.cureVideoBytes,
            manifest.cureVideoFrames,
        },
        {
            tempBase + QStringLiteral(".avi"),
            manifest.temperatureVideoBytes,
            manifest.temperatureVideoFrames,
        },
        {
            stressBase + QStringLiteral(".avi"),
            manifest.stressVideoBytes,
            manifest.stressVideoFrames,
        },
    };

    for (const auto &aviSet : aviSets) {
        const QFileInfo info(aviSet.aviPath);
        if (!info.exists() || info.size() <= 0) {
            errorMessage =
                QStringLiteral("AVI 缺失或为空：%1").arg(aviSet.aviPath);
            return false;
        }

        if (info.size() != aviSet.expectedBytes) {
            errorMessage =
                QStringLiteral(
                    "AVI 文件大小不匹配：%1（期望 %2 字节，实际 %3 字节）"
                ).arg(aviSet.aviPath)
                    .arg(aviSet.expectedBytes)
                    .arg(info.size());
            return false;
        }

        if (aviSet.expectedVideoFrames != expectedFrames) {
            errorMessage =
                QStringLiteral(
                    "AVI 帧数记录不匹配：%1（期望 %2）"
                ).arg(aviSet.aviPath).arg(expectedFrames);
            return false;
        }
    }

    return true;
}

bool clearPostProcessOutputs(
    const QString &projectPath,
    QString &errorMessage)
{
    const QString resultsDir = resultsDirectory(projectPath);
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

        const QStringList suffixes = {
            QStringLiteral(".avi"),
            QStringLiteral(".tmp.avi"),
        };

        for (const QString &suffix : suffixes) {
            const QString path =
                QDir(resultsDir).filePath(baseName + suffix);

            if (QFileInfo::exists(path) && !QFile::remove(path)) {
                errorMessage =
                    QStringLiteral("无法删除后处理文件：\n%1").arg(path);
                return false;
            }
        }
    }

    QDir resultsDirListing(resultsDir);
    if (resultsDirListing.exists()) {
        const QStringList tmpAviFiles =
            resultsDirListing.entryList(
                QStringList() << QStringLiteral("*.tmp.avi"),
                QDir::Files);

        for (const QString &name : tmpAviFiles) {
            const QString path = resultsDirListing.filePath(name);

            if (!QFile::remove(path)) {
                errorMessage =
                    QStringLiteral("无法删除后处理临时文件：\n%1").arg(path);
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

    const QString t2FlagPath = t2FinishedFlagPath(projectPath);
    if (QFileInfo::exists(t2FlagPath) && !QFile::remove(t2FlagPath)) {
        errorMessage =
            QStringLiteral("无法删除后处理完成标志：\n%1").arg(t2FlagPath);
        return false;
    }

    errorMessage.clear();
    return true;
}

QString runningInputFingerprintPath(const QString &projectPath)
{
    return ProjectPaths::runningInputFingerprintPath(projectPath);
}

QString runningInputPrepareFingerprintPath(const QString &projectPath)
{
    return ProjectPaths::runningInputPrepareFingerprintPath(projectPath);
}

QString lastSuccessInputFingerprintPath(const QString &projectPath)
{
    return ProjectPaths::lastSuccessInputFingerprintPath(projectPath);
}

QString runningPostFingerprintPath(const QString &projectPath)
{
    return ProjectPaths::runningPostFingerprintPath(projectPath);
}

QString lastSuccessPostFingerprintPath(const QString &projectPath)
{
    return ProjectPaths::lastSuccessPostFingerprintPath(projectPath);
}

QString t1FinishedFlagPath(const QString &projectPath)
{
    return ProjectPaths::t1FinishedFlagPath(projectPath);
}

QString t2FinishedFlagPath(const QString &projectPath)
{
    return ProjectPaths::t2FinishedFlagPath(projectPath);
}

QString solverOdbPath(const QString &projectPath)
{
    return ProjectPaths::solverOdbPath(projectPath);
}

QString currentJobName(const QString &projectPath)
{
    return ProjectPaths::currentJobName(projectPath);
}

QString currentJobLockPath(const QString &projectPath)
{
    return ProjectPaths::currentJobLockPath(projectPath);
}

} // namespace ProjectInputHash
