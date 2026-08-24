#include "StructureConfigManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace {
const QString FILE_STRUCTURE = QStringLiteral("structure.json");
const int STRUCTURE_SCHEMA_VERSION = 1;

QString structureFilePath(const QString &projectPath)
{
    return QDir(projectPath).filePath(QStringLiteral("config/") + FILE_STRUCTURE);
}
}

bool StructureConfigManager::validate(
    const StructureConfig &config,
    QString &errorMessage)
{
    if (config.chargeRadius <= 0.0) {
        errorMessage = QStringLiteral("药柱半径必须大于 0。");
        return false;
    }

    if (config.chargeHeight <= 0.0) {
        errorMessage = QStringLiteral("药柱高度必须大于 0。");
        return false;
    }

    if (config.shellThickness <= 0.0) {
        errorMessage = QStringLiteral("外壳厚度必须大于 0。");
        return false;
    }

    return true;
}

bool StructureConfigManager::save(
    const QString &projectPath,
    const StructureConfig &config)
{
    QString errorMessage;
    if (!validate(config, errorMessage)) {
        return false;
    }

    QDir configDir(QDir(projectPath).filePath(QStringLiteral("config")));
    if (!configDir.exists() && !QDir(projectPath).mkpath(QStringLiteral("config"))) {
        return false;
    }

    QJsonObject jsonObj;
    jsonObj[QStringLiteral("schemaVersion")] = STRUCTURE_SCHEMA_VERSION;
    jsonObj[QStringLiteral("chargeRadius")] = config.chargeRadius;
    jsonObj[QStringLiteral("chargeHeight")] = config.chargeHeight;
    jsonObj[QStringLiteral("shellThickness")] = config.shellThickness;

    QSaveFile file(structureFilePath(projectPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool StructureConfigManager::load(
    const QString &projectPath,
    StructureConfig &config)
{
    QFile file(structureFilePath(projectPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!document.isObject()) {
        return false;
    }

    const QJsonObject jsonObj = document.object();
    const int schemaVersion = jsonObj.value(QStringLiteral("schemaVersion")).toInt(-1);
    if (schemaVersion != STRUCTURE_SCHEMA_VERSION) {
        return false;
    }

    if (!jsonObj.contains(QStringLiteral("chargeRadius"))
        || !jsonObj.contains(QStringLiteral("chargeHeight"))
        || !jsonObj.contains(QStringLiteral("shellThickness"))) {
        return false;
    }

    StructureConfig loaded;
    loaded.schemaVersion = schemaVersion;
    loaded.chargeRadius = jsonObj.value(QStringLiteral("chargeRadius")).toDouble();
    loaded.chargeHeight = jsonObj.value(QStringLiteral("chargeHeight")).toDouble();
    loaded.shellThickness = jsonObj.value(QStringLiteral("shellThickness")).toDouble();

    QString errorMessage;
    if (!validate(loaded, errorMessage)) {
        return false;
    }

    config = loaded;
    return true;
}
