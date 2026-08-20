#include "BoundaryConfigManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace {
const QString FILE_BOUNDARY = QStringLiteral("boundary.json");
const int BOUNDARY_SCHEMA_VERSION = 1;

QString boundaryFilePath(const QString &projectPath)
{
    return QDir(projectPath).filePath(QStringLiteral("config/") + FILE_BOUNDARY);
}
}

bool BoundaryConfigManager::validate(
    const BoundaryConfig &config,
    QString &errorMessage)
{
    if (config.ambientTemperature <= 0.0) {
        errorMessage = QStringLiteral("环境温度必须大于 0。");
        return false;
    }

    return true;
}

bool BoundaryConfigManager::save(
    const QString &projectPath,
    const BoundaryConfig &config)
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
    jsonObj[QStringLiteral("schemaVersion")] = BOUNDARY_SCHEMA_VERSION;
    jsonObj[QStringLiteral("ambientTemperature")] = config.ambientTemperature;

    QSaveFile file(boundaryFilePath(projectPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool BoundaryConfigManager::load(
    const QString &projectPath,
    BoundaryConfig &config)
{
    QFile file(boundaryFilePath(projectPath));
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
    if (schemaVersion != BOUNDARY_SCHEMA_VERSION) {
        return false;
    }

    if (!jsonObj.contains(QStringLiteral("ambientTemperature"))) {
        return false;
    }

    BoundaryConfig loaded;
    loaded.schemaVersion = schemaVersion;
    loaded.ambientTemperature = jsonObj.value(QStringLiteral("ambientTemperature")).toDouble();

    QString errorMessage;
    if (!validate(loaded, errorMessage)) {
        return false;
    }

    config = loaded;
    return true;
}
