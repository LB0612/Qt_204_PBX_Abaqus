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
constexpr int kPostProcessIntegrityVersion = 2;

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
    QJsonObject &stored,
    bool &metadataChanged,
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

    // 内容完全相同，只是 size/mtime 等元数据发生过变化。
    // 刷新 metadata，避免下次再次完整计算所有 PNG 的 SHA。
    stored[QStringLiteral("metadataSha256")] =
        currentMetadataSha;

    metadataChanged = true;

    return true;
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

    // SHA 一致，说明 ODB 内容实际没有变化。
    // 刷新元数据，让之后重新回到快速校验路径。
    json[QStringLiteral("odbBytes")] =
        static_cast<double>(odbInfo.size());

    json[QStringLiteral("odbLastModifiedMs")] =
        QString::number(
            odbInfo.lastModified().toMSecsSinceEpoch()
        );

    // 这里只是刷新缓存元数据。
    // 即使写回失败，也不能把一个已经通过 SHA
    // 验证的正确 ODB 判成损坏。
    QString refreshError;
    writeJsonFile(
        integrityPath,
        json,
        refreshError
    );

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

    const QJsonObject json = {
        {QStringLiteral("version"), kPostProcessIntegrityVersion},
        {QStringLiteral("postSha256"), postSha256},
        {QStringLiteral("curePng"), curePng},
        {QStringLiteral("temperaturePng"), temperaturePng},
        {QStringLiteral("stressPng"), stressPng},
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

    bool metadataChanged = false;

    QJsonObject curePng =
        json.value(
            QStringLiteral("curePng")
        ).toObject();

    QJsonObject temperaturePng =
        json.value(
            QStringLiteral("temperaturePng")
        ).toObject();

    QJsonObject stressPng =
        json.value(
            QStringLiteral("stressPng")
        ).toObject();

    if (!validatePngIntegrityObject(
            projectPath,
            ResultType::Cure,
            curePng,
            metadataChanged,
            errorMessage)) {
        return false;
    }

    if (!validatePngIntegrityObject(
            projectPath,
            ResultType::Temperature,
            temperaturePng,
            metadataChanged,
            errorMessage)) {
        return false;
    }

    if (!validatePngIntegrityObject(
            projectPath,
            ResultType::Stress,
            stressPng,
            metadataChanged,
            errorMessage)) {
        return false;
    }

    if (metadataChanged) {
        json[QStringLiteral("curePng")] =
            curePng;

        json[QStringLiteral("temperaturePng")] =
            temperaturePng;

        json[QStringLiteral("stressPng")] =
            stressPng;

        // 元数据刷新属于性能缓存更新。
        // 文件内容已经通过 SHA 校验，因此写回失败
        // 不应该把正确结果判为损坏。
        QString refreshError;
        writeJsonFile(
            integrityPath,
            json,
            refreshError
        );
    }

    errorMessage.clear();
    return true;
}
