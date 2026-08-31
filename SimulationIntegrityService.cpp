#include "SimulationIntegrityService.h"

#include "ProjectInputHash.h"
#include "ProjectPaths.h"
#include "SimulationArtifactStateService.h"
#include "SimulationResultService.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {

constexpr int kSolverIntegrityVersion = 1;
constexpr int kPostProcessIntegrityVersion = 1;

bool writeJsonFile(
    const QString &path,
    const QJsonObject &json,
    QString &errorMessage)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errorMessage =
            QStringLiteral("无法写入完整性记录：\n%1").arg(path);
        return false;
    }

    file.write(
        QJsonDocument(json).toJson(QJsonDocument::Indented)
    );

    if (!file.commit()) {
        errorMessage =
            QStringLiteral("完整性记录保存失败：\n%1").arg(path);
        return false;
    }

    return true;
}

bool readJsonObject(
    const QString &path,
    QJsonObject &json,
    QString &errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage =
            QStringLiteral("无法读取完整性记录：\n%1").arg(path);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        errorMessage =
            QStringLiteral("完整性记录格式无效：\n%1").arg(path);
        return false;
    }

    json = document.object();
    return true;
}

QString hashFileContent(const QString &path)
{
    return SimulationArtifactStateService::fileSha256(path);
}

QString hashPngGroupMetadata(
    const QString &projectPath,
    ResultType type,
    int frameCount)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);

    for (int index = 0; index < frameCount; ++index) {
        const QString path =
            SimulationResultService::framePngPath(
                projectPath,
                type,
                index
            );
        const QFileInfo info(path);
        const QString fileName = info.fileName();

        hash.addData(fileName.toUtf8());
        hash.addData("\n");
        hash.addData(
            QByteArray::number(info.size())
        );
        hash.addData("\n");
        hash.addData(
            QByteArray::number(
                info.lastModified().toMSecsSinceEpoch()
            )
        );
        hash.addData("\n");
    }

    return QString::fromLatin1(hash.result().toHex());
}

QString hashPngGroupContent(
    const QString &projectPath,
    ResultType type,
    int frameCount,
    QString &errorMessage)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);

    for (int index = 0; index < frameCount; ++index) {
        const QString path =
            SimulationResultService::framePngPath(
                projectPath,
                type,
                index
            );

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            errorMessage =
                QStringLiteral("无法读取 PNG：\n%1").arg(path);
            return QString();
        }

        hash.addData(QFileInfo(path).fileName().toUtf8());
        hash.addData("\n");

        constexpr qint64 kChunkSize = 1024 * 1024;

        while (!file.atEnd()) {
            const QByteArray chunk = file.read(kChunkSize);

            if (chunk.isEmpty()) {
                if (file.error() != QFile::NoError) {
                    errorMessage =
                        QStringLiteral("无法读取 PNG：\n%1")
                            .arg(path);
                    return QString();
                }
                break;
            }

            hash.addData(chunk);
        }

        hash.addData("\n");
    }

    errorMessage.clear();
    return QString::fromLatin1(hash.result().toHex());
}

QJsonObject buildPngIntegrityObject(
    const QString &projectPath,
    ResultType type,
    int frameCount,
    QString &errorMessage)
{
    const QString metadataSha =
        hashPngGroupMetadata(projectPath, type, frameCount);
    const QString contentSha =
        hashPngGroupContent(
            projectPath,
            type,
            frameCount,
            errorMessage
        );

    if (contentSha.isEmpty()) {
        return QJsonObject();
    }

    return {
        {QStringLiteral("count"), frameCount},
        {QStringLiteral("metadataSha256"), metadataSha},
        {QStringLiteral("contentSha256"), contentSha},
    };
}

bool validatePngIntegrityObject(
    const QString &projectPath,
    ResultType type,
    const QJsonObject &stored,
    QString &errorMessage)
{
    const int storedCount =
        stored.value(QStringLiteral("count")).toInt(-1);
    const QString storedMetadataSha =
        stored.value(QStringLiteral("metadataSha256")).toString();
    const QString storedContentSha =
        stored.value(QStringLiteral("contentSha256")).toString();

    if (storedCount <= 0
        || storedMetadataSha.isEmpty()
        || storedContentSha.isEmpty()) {
        errorMessage = QStringLiteral("PNG 完整性记录无效。");
        return false;
    }

    const QString currentMetadataSha =
        hashPngGroupMetadata(projectPath, type, storedCount);

    if (currentMetadataSha == storedMetadataSha) {
        return true;
    }

    QString contentError;
    const QString currentContentSha =
        hashPngGroupContent(
            projectPath,
            type,
            storedCount,
            contentError
        );

    if (currentContentSha.isEmpty()) {
        errorMessage = contentError.isEmpty()
            ? QStringLiteral("PNG 完整性校验失败。")
            : contentError;
        return false;
    }

    if (currentContentSha != storedContentSha) {
        errorMessage = QStringLiteral("PNG 内容完整性校验失败。");
        return false;
    }

    return true;
}

QJsonObject buildAviIntegrityObject(
    const QString &aviPath,
    QString &errorMessage)
{
    const QFileInfo info(aviPath);
    if (!info.exists() || !info.isFile() || info.size() <= 0) {
        errorMessage =
            QStringLiteral("AVI 缺失或为空：\n%1").arg(aviPath);
        return QJsonObject();
    }

    const QString sha = hashFileContent(aviPath);
    if (sha.isEmpty()) {
        errorMessage =
            QStringLiteral("无法计算 AVI 校验值：\n%1").arg(aviPath);
        return QJsonObject();
    }

    return {
        {
            QStringLiteral("bytes"),
            static_cast<double>(info.size())
        },
        {
            QStringLiteral("lastModifiedMs"),
            QString::number(info.lastModified().toMSecsSinceEpoch())
        },
        {QStringLiteral("sha256"), sha},
    };
}

bool validateAviIntegrityObject(
    const QString &aviPath,
    const QJsonObject &stored,
    QString &errorMessage)
{
    const qint64 storedBytes =
        static_cast<qint64>(
            stored.value(QStringLiteral("bytes")).toDouble(-1)
        );
    const QString storedMtime =
        stored.value(QStringLiteral("lastModifiedMs")).toString();
    const QString storedSha =
        stored.value(QStringLiteral("sha256")).toString();

    if (storedBytes <= 0
        || storedMtime.isEmpty()
        || storedSha.isEmpty()) {
        errorMessage = QStringLiteral("AVI 完整性记录无效。");
        return false;
    }

    const QFileInfo info(aviPath);
    if (!info.exists() || !info.isFile() || info.size() <= 0) {
        errorMessage =
            QStringLiteral("AVI 缺失或为空：\n%1").arg(aviPath);
        return false;
    }

    if (info.size() == storedBytes
        && QString::number(info.lastModified().toMSecsSinceEpoch())
            == storedMtime) {
        return true;
    }

    const QString currentSha = hashFileContent(aviPath);
    if (currentSha.isEmpty()) {
        errorMessage =
            QStringLiteral("无法计算 AVI 校验值：\n%1").arg(aviPath);
        return false;
    }

    if (currentSha != storedSha || info.size() != storedBytes) {
        errorMessage =
            QStringLiteral("AVI 完整性校验失败：\n%1").arg(aviPath);
        return false;
    }

    return true;
}

QString aviPathForBase(
    const QString &projectPath,
    const QString &baseName)
{
    return QDir(
        ProjectPaths::resultsDirectoryPath(projectPath)
    ).filePath(baseName + QStringLiteral(".avi"));
}

} // namespace

bool SimulationIntegrityService::writeSolverResultIntegrity(
    const QString &projectPath,
    const QString &inputSha256,
    QString &errorMessage)
{
    if (projectPath.isEmpty() || inputSha256.isEmpty()) {
        errorMessage = QStringLiteral("求解完整性参数无效。");
        return false;
    }

    const QString odbPath =
        ProjectPaths::solverOdbPath(projectPath);
    const QFileInfo odbInfo(odbPath);

    if (!odbInfo.exists() || !odbInfo.isFile() || odbInfo.size() <= 0) {
        errorMessage =
            QStringLiteral("无法记录 ODB 完整性：文件无效。");
        return false;
    }

    const QString odbSha = hashFileContent(odbPath);
    if (odbSha.isEmpty()) {
        errorMessage =
            QStringLiteral("无法计算 ODB 校验值。");
        return false;
    }

    const QJsonObject json = {
        {QStringLiteral("version"), kSolverIntegrityVersion},
        {QStringLiteral("inputSha256"), inputSha256},
        {QStringLiteral("odbFileName"), odbInfo.fileName()},
        {
            QStringLiteral("odbBytes"),
            static_cast<double>(odbInfo.size())
        },
        {
            QStringLiteral("odbLastModifiedMs"),
            QString::number(odbInfo.lastModified().toMSecsSinceEpoch())
        },
        {QStringLiteral("odbSha256"), odbSha},
    };

    return writeJsonFile(
        ProjectPaths::solverResultIntegrityPath(projectPath),
        json,
        errorMessage
    );
}

bool SimulationIntegrityService::validateSolverResultIntegrity(
    const QString &projectPath,
    const QString &inputSha256,
    QString &errorMessage,
    bool allowBootstrap)
{
    const QString integrityPath =
        ProjectPaths::solverResultIntegrityPath(projectPath);

    if (!QFileInfo::exists(integrityPath)) {
        if (allowBootstrap) {
            return writeSolverResultIntegrity(
                projectPath,
                inputSha256,
                errorMessage
            );
        }

        errorMessage =
            QStringLiteral("缺少求解结果完整性记录。");
        return false;
    }

    QJsonObject json;
    if (!readJsonObject(integrityPath, json, errorMessage)) {
        return false;
    }

    if (json.value(QStringLiteral("version")).toInt() != kSolverIntegrityVersion) {
        errorMessage =
            QStringLiteral("求解结果完整性记录版本不匹配。");
        return false;
    }

    if (json.value(QStringLiteral("inputSha256")).toString()
        != inputSha256) {
        errorMessage =
            QStringLiteral("求解结果完整性记录与当前输入不一致。");
        return false;
    }

    const QString odbPath =
        ProjectPaths::solverOdbPath(projectPath);
    const QFileInfo odbInfo(odbPath);

    if (!odbInfo.exists() || !odbInfo.isFile() || odbInfo.size() <= 0) {
        errorMessage =
            QStringLiteral("ODB 文件无效或已丢失。");
        return false;
    }

    if (odbInfo.fileName()
        != json.value(QStringLiteral("odbFileName")).toString()) {
        errorMessage =
            QStringLiteral("ODB 文件名与完整性记录不一致。");
        return false;
    }

    const qint64 storedBytes =
        static_cast<qint64>(
            json.value(QStringLiteral("odbBytes")).toDouble(-1)
        );
    const QString storedMtime =
        json.value(QStringLiteral("odbLastModifiedMs")).toString();
    const QString storedSha =
        json.value(QStringLiteral("odbSha256")).toString();

    if (storedBytes <= 0
        || storedMtime.isEmpty()
        || storedSha.isEmpty()) {
        errorMessage =
            QStringLiteral("求解结果完整性记录内容无效。");
        return false;
    }

    if (odbInfo.size() == storedBytes
        && QString::number(odbInfo.lastModified().toMSecsSinceEpoch())
            == storedMtime) {
        errorMessage.clear();
        return true;
    }

    const QString currentSha = hashFileContent(odbPath);
    if (currentSha.isEmpty()) {
        errorMessage =
            QStringLiteral("无法重新计算 ODB 校验值。");
        return false;
    }

    if (currentSha != storedSha || odbInfo.size() != storedBytes) {
        errorMessage =
            QStringLiteral("ODB 文件内容与完整性记录不一致。");
        return false;
    }

    errorMessage.clear();
    return true;
}

bool SimulationIntegrityService::writePostProcessIntegrity(
    const QString &projectPath,
    const QString &postSha256,
    QString &errorMessage)
{
    if (projectPath.isEmpty() || postSha256.isEmpty()) {
        errorMessage = QStringLiteral("后处理完整性参数无效。");
        return false;
    }

    const ProjectInputHash::PostProcessManifest manifest =
        ProjectInputHash::readPostProcessManifest(projectPath);
    if (!manifest.valid || manifest.odbFrames <= 0) {
        errorMessage =
            QStringLiteral("后处理清单无效，无法建立完整性记录。");
        return false;
    }

    const int frameCount = manifest.odbFrames;

    QJsonObject curePng =
        buildPngIntegrityObject(
            projectPath,
            ResultType::Cure,
            frameCount,
            errorMessage
        );
    if (curePng.isEmpty()) {
        return false;
    }

    QJsonObject temperaturePng =
        buildPngIntegrityObject(
            projectPath,
            ResultType::Temperature,
            frameCount,
            errorMessage
        );
    if (temperaturePng.isEmpty()) {
        return false;
    }

    QJsonObject stressPng =
        buildPngIntegrityObject(
            projectPath,
            ResultType::Stress,
            frameCount,
            errorMessage
        );
    if (stressPng.isEmpty()) {
        return false;
    }

    QJsonObject cureAvi =
        buildAviIntegrityObject(
            aviPathForBase(projectPath, QStringLiteral("guhuadu")),
            errorMessage
        );
    if (cureAvi.isEmpty()) {
        return false;
    }

    QJsonObject temperatureAvi =
        buildAviIntegrityObject(
            aviPathForBase(projectPath, QStringLiteral("wendu")),
            errorMessage
        );
    if (temperatureAvi.isEmpty()) {
        return false;
    }

    QJsonObject stressAvi =
        buildAviIntegrityObject(
            aviPathForBase(projectPath, QStringLiteral("yingli")),
            errorMessage
        );
    if (stressAvi.isEmpty()) {
        return false;
    }

    const QJsonObject json = {
        {QStringLiteral("version"), kPostProcessIntegrityVersion},
        {QStringLiteral("postSha256"), postSha256},
        {QStringLiteral("curePng"), curePng},
        {QStringLiteral("temperaturePng"), temperaturePng},
        {QStringLiteral("stressPng"), stressPng},
        {QStringLiteral("cureAvi"), cureAvi},
        {QStringLiteral("temperatureAvi"), temperatureAvi},
        {QStringLiteral("stressAvi"), stressAvi},
    };

    return writeJsonFile(
        ProjectPaths::postProcessIntegrityPath(projectPath),
        json,
        errorMessage
    );
}

bool SimulationIntegrityService::validatePostProcessIntegrity(
    const QString &projectPath,
    const QString &postSha256,
    QString &errorMessage,
    bool allowBootstrap)
{
    const QString integrityPath =
        ProjectPaths::postProcessIntegrityPath(projectPath);

    if (!QFileInfo::exists(integrityPath)) {
        if (allowBootstrap) {
            return writePostProcessIntegrity(
                projectPath,
                postSha256,
                errorMessage
            );
        }

        errorMessage =
            QStringLiteral("缺少后处理完整性记录。");
        return false;
    }

    QJsonObject json;
    if (!readJsonObject(integrityPath, json, errorMessage)) {
        return false;
    }

    if (json.value(QStringLiteral("version")).toInt()
        != kPostProcessIntegrityVersion) {
        errorMessage =
            QStringLiteral("后处理完整性记录版本不匹配。");
        return false;
    }

    if (json.value(QStringLiteral("postSha256")).toString()
        != postSha256) {
        errorMessage =
            QStringLiteral("后处理完整性记录与当前输入不一致。");
        return false;
    }

    if (!validatePngIntegrityObject(
            projectPath,
            ResultType::Cure,
            json.value(QStringLiteral("curePng")).toObject(),
            errorMessage)) {
        return false;
    }

    if (!validatePngIntegrityObject(
            projectPath,
            ResultType::Temperature,
            json.value(QStringLiteral("temperaturePng")).toObject(),
            errorMessage)) {
        return false;
    }

    if (!validatePngIntegrityObject(
            projectPath,
            ResultType::Stress,
            json.value(QStringLiteral("stressPng")).toObject(),
            errorMessage)) {
        return false;
    }

    if (!validateAviIntegrityObject(
            aviPathForBase(projectPath, QStringLiteral("guhuadu")),
            json.value(QStringLiteral("cureAvi")).toObject(),
            errorMessage)) {
        return false;
    }

    if (!validateAviIntegrityObject(
            aviPathForBase(projectPath, QStringLiteral("wendu")),
            json.value(QStringLiteral("temperatureAvi")).toObject(),
            errorMessage)) {
        return false;
    }

    if (!validateAviIntegrityObject(
            aviPathForBase(projectPath, QStringLiteral("yingli")),
            json.value(QStringLiteral("stressAvi")).toObject(),
            errorMessage)) {
        return false;
    }

    errorMessage.clear();
    return true;
}
