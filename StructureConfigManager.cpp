#include "StructureConfigManager.h"

#include "ProjectPaths.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <cmath>

namespace {
const int STRUCTURE_SCHEMA_VERSION = 1;
}

bool StructureConfigManager::validate(
    const StructureConfig &config,
    QString &errorMessage)
{
    if (!std::isfinite(config.chargeRadius)
        || config.chargeRadius <= 0.0) {
        errorMessage = QStringLiteral("药柱半径必须是有限数值且大于 0。");
        return false;
    }

    if (!std::isfinite(config.chargeHeight)
        || config.chargeHeight <= 0.0) {
        errorMessage = QStringLiteral("药柱高度必须是有限数值且大于 0。");
        return false;
    }

    if (!std::isfinite(config.shellThickness)
        || config.shellThickness <= 0.0) {
        errorMessage = QStringLiteral("外壳厚度必须是有限数值且大于 0。");
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

    const QString configDirPath =
        ProjectPaths::configDirectoryPath(projectPath);
    if (!QDir(configDirPath).exists()
        && !QDir().mkpath(configDirPath)) {
        return false;
    }

    QJsonObject jsonObj;
    jsonObj[QStringLiteral("schemaVersion")] = STRUCTURE_SCHEMA_VERSION;
    jsonObj[QStringLiteral("chargeRadius")] = config.chargeRadius;
    jsonObj[QStringLiteral("chargeHeight")] = config.chargeHeight;
    jsonObj[QStringLiteral("shellThickness")] = config.shellThickness;

    QSaveFile file(ProjectPaths::structureConfigPath(projectPath));
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
    QFile file(ProjectPaths::structureConfigPath(projectPath));
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
