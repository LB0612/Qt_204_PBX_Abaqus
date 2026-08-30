#include "SimulationConfigManager.h"

#include "ProjectPaths.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace {
const int SIMULATION_SCHEMA_VERSION = 1;
}

bool SimulationConfigManager::validate(
    const SimulationConfig &config,
    QString &errorMessage)
{
    if (config.timeLength <= 0.0) {
        errorMessage = QStringLiteral("时间长度必须大于 0。");
        return false;
    }

    return true;
}

bool SimulationConfigManager::save(
    const QString &projectPath,
    const SimulationConfig &config)
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
    jsonObj[QStringLiteral("schemaVersion")] = SIMULATION_SCHEMA_VERSION;
    jsonObj[QStringLiteral("timeLength")] = config.timeLength;

    QSaveFile file(ProjectPaths::simulationConfigPath(projectPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool SimulationConfigManager::load(
    const QString &projectPath,
    SimulationConfig &config)
{
    QFile file(ProjectPaths::simulationConfigPath(projectPath));
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
    if (schemaVersion != SIMULATION_SCHEMA_VERSION) {
        return false;
    }

    if (!jsonObj.contains(QStringLiteral("timeLength"))) {
        return false;
    }

    SimulationConfig loaded;
    loaded.schemaVersion = schemaVersion;
    loaded.timeLength = jsonObj.value(QStringLiteral("timeLength")).toDouble();

    QString errorMessage;
    if (!validate(loaded, errorMessage)) {
        return false;
    }

    config = loaded;
    return true;
}
