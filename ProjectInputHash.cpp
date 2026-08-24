#include "ProjectInputHash.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
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
