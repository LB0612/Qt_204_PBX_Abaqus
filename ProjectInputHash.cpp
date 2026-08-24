#include "ProjectInputHash.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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

bool runVersionCheck(
    const QString &executablePath,
    QString &errorMessage)
{
    QProcess process;
    process.start(executablePath, {QStringLiteral("-version")});

    if (!process.waitForStarted(5000)) {
        errorMessage =
            QStringLiteral("无法启动后处理组件：%1").arg(executablePath);
        return false;
    }

    if (!process.waitForFinished(10000)) {
        process.kill();
        process.waitForFinished(3000);
        errorMessage =
            QStringLiteral("后处理组件无响应：%1").arg(executablePath);
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        errorMessage =
            QStringLiteral("后处理组件版本检查失败：%1").arg(executablePath);
        return false;
    }

    return true;
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

    manifest.valid =
        manifest.version == 1
        && !manifest.postSha256.isEmpty()
        && countsMatch;

    return manifest;
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

QString bundledFfmpegPath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("tools/ffmpeg/ffmpeg.exe"));
}

QString bundledFfprobePath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("tools/ffmpeg/ffprobe.exe"));
}

bool bundledFfmpegAvailable(QString &errorMessage)
{
    const QString ffmpegPath = bundledFfmpegPath();
    const QString ffprobePath = bundledFfprobePath();

    if (!QFile::exists(ffmpegPath)) {
        errorMessage =
            QStringLiteral(
                "后处理组件缺失：\n%1\n\n"
                "请重新安装完整程序。"
            ).arg(ffmpegPath);
        return false;
    }

    if (!QFile::exists(ffprobePath)) {
        errorMessage =
            QStringLiteral(
                "后处理组件缺失：\n%1\n\n"
                "请重新安装完整程序。"
            ).arg(ffprobePath);
        return false;
    }

    if (!runVersionCheck(ffmpegPath, errorMessage)) {
        return false;
    }

    if (!runVersionCheck(ffprobePath, errorMessage)) {
        return false;
    }

    return true;
}

} // namespace ProjectInputHash
