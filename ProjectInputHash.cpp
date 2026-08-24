#include "ProjectInputHash.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace ProjectInputHash {

namespace {

QStringList configRelativePaths()
{
    return {
        QStringLiteral("config/structure.json"),
        QStringLiteral("config/explosive.json"),
        QStringLiteral("config/mold.json"),
        QStringLiteral("config/boundary.json"),
        QStringLiteral("config/simulation.json"),
    };
}

QStringList generatedAbaqusRelativePaths()
{
    return {
        QStringLiteral("abaqus/t0.py"),
        QStringLiteral("abaqus/t1.py"),
        QStringLiteral("abaqus/t2.py"),
        QStringLiteral("abaqus/335K.for"),
    };
}

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

QString generationFlagPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("generation_complete.flag"));
}

QString postProcessManifestPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("results")))
        .filePath(QStringLiteral("postprocess_manifest.json"));
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
    return hashRelativeFiles(projectPath, configRelativePaths());
}

QString hashGeneratedFiles(const QString &projectPath)
{
    QStringList paths = configRelativePaths();
    paths += generatedAbaqusRelativePaths();
    return hashRelativeFiles(projectPath, paths);
}

QString hashSolverInput(const QString &projectPath)
{
    if (projectPath.isEmpty()) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    const QDir projectDir(projectPath);

    QStringList paths = configRelativePaths();
    paths += {
        QStringLiteral("abaqus/t0.py"),
        QStringLiteral("abaqus/t1.py"),
        QStringLiteral("abaqus/335K.for"),
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

    QFile t2File(
        QDir(projectPath).filePath(QStringLiteral("abaqus/t2.py"))
    );
    if (!t2File.open(QIODevice::ReadOnly)) {
        return QString();
    }

    hash.addData(QStringLiteral("abaqus/t2.py").toUtf8());
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

    QFile file(generationFlagPath(projectPath));
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
        }
    }

    manifest.valid =
        manifest.version == GENERATION_MANIFEST_VERSION
        && manifest.status == QStringLiteral("success")
        && !manifest.configSha256.isEmpty()
        && !manifest.generatedSha256.isEmpty();

    return manifest;
}

bool writeGenerationManifest(
    const QString &projectPath,
    const QString &configSha256,
    const QString &generatedSha256,
    QString &errorMessage)
{
    const QString manifestText =
        QStringLiteral("version=%1\n"
                       "status=success\n"
                       "config_sha256=%2\n"
                       "generated_sha256=%3\n")
            .arg(GENERATION_MANIFEST_VERSION)
            .arg(configSha256)
            .arg(generatedSha256);

    QSaveFile flagFile(generationFlagPath(projectPath));
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

    QFile file(postProcessManifestPath(projectPath));
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

    manifest.valid =
        manifest.version == 2
        && !manifest.postSha256.isEmpty()
        && countsMatch
        && bytesRecorded;

    return manifest;
}

QString resultsDirectory(const QString &projectPath)
{
    return QDir(projectPath).filePath(QStringLiteral("results"));
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

QString runningInputFingerprintPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("running_input.sha256"));
}

QString lastSuccessInputFingerprintPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("last_success_input.sha256"));
}

QString runningPostFingerprintPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("results")))
        .filePath(QStringLiteral("running_post_input.sha256"));
}

QString lastSuccessPostFingerprintPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("results")))
        .filePath(QStringLiteral("last_success_post.sha256"));
}

QString t1FinishedFlagPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("t1_finished.flag"));
}

QString t2FinishedFlagPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("t2_finished.flag"));
}

QString solverOdbPath(const QString &projectPath)
{
    const QString jobName = currentJobName(projectPath);
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(jobName + QStringLiteral(".odb"));
}

QString currentJobName(const QString &projectPath)
{
    return QDir(projectPath).dirName() + QStringLiteral("_Job");
}

QString currentJobLockPath(const QString &projectPath)
{
    return QDir(QDir(projectPath).filePath(QStringLiteral("abaqus")))
        .filePath(currentJobName(projectPath) + QStringLiteral(".lck"));
}

} // namespace ProjectInputHash
