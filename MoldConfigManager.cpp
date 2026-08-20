#include "MoldConfigManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace {
const QString FILE_MOLD = QStringLiteral("mold.json");
const int MOLD_SCHEMA_VERSION = 1;

QString moldFilePath(const QString &projectPath)
{
    return QDir(projectPath).filePath(QStringLiteral("config/") + FILE_MOLD);
}
}

bool MoldConfigManager::validate(
    const MoldConfig &config,
    QString &errorMessage)
{
    if (config.density <= 0.0) {
        errorMessage = QStringLiteral("模具密度必须大于 0。");
        return false;
    }

    if (config.elasticModulus <= 0.0) {
        errorMessage = QStringLiteral("模具弹性模量必须大于 0。");
        return false;
    }

    if (config.poissonRatio <= 0.0 || config.poissonRatio >= 0.5) {
        errorMessage = QStringLiteral("模具泊松比必须大于 0 且小于 0.5。");
        return false;
    }

    if (config.thermalConductivity <= 0.0) {
        errorMessage = QStringLiteral("模具热导率必须大于 0。");
        return false;
    }

    if (config.specificHeat <= 0.0) {
        errorMessage = QStringLiteral("模具比热容必须大于 0。");
        return false;
    }

    return true;
}

bool MoldConfigManager::save(
    const QString &projectPath,
    const MoldConfig &config)
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
    jsonObj[QStringLiteral("schemaVersion")] = MOLD_SCHEMA_VERSION;
    jsonObj[QStringLiteral("density")] = config.density;
    jsonObj[QStringLiteral("elasticModulus")] = config.elasticModulus;
    jsonObj[QStringLiteral("poissonRatio")] = config.poissonRatio;
    jsonObj[QStringLiteral("thermalConductivity")] = config.thermalConductivity;
    jsonObj[QStringLiteral("specificHeat")] = config.specificHeat;

    QSaveFile file(moldFilePath(projectPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool MoldConfigManager::load(
    const QString &projectPath,
    MoldConfig &config)
{
    QFile file(moldFilePath(projectPath));
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
    if (schemaVersion != MOLD_SCHEMA_VERSION) {
        return false;
    }

    const QStringList requiredKeys = {
        QStringLiteral("density"),
        QStringLiteral("elasticModulus"),
        QStringLiteral("poissonRatio"),
        QStringLiteral("thermalConductivity"),
        QStringLiteral("specificHeat")
    };

    for (const QString &key : requiredKeys) {
        if (!jsonObj.contains(key)) {
            return false;
        }
    }

    MoldConfig loaded;
    loaded.schemaVersion = schemaVersion;
    loaded.density = jsonObj.value(QStringLiteral("density")).toDouble();
    loaded.elasticModulus = jsonObj.value(QStringLiteral("elasticModulus")).toDouble();
    loaded.poissonRatio = jsonObj.value(QStringLiteral("poissonRatio")).toDouble();
    loaded.thermalConductivity = jsonObj.value(QStringLiteral("thermalConductivity")).toDouble();
    loaded.specificHeat = jsonObj.value(QStringLiteral("specificHeat")).toDouble();

    QString errorMessage;
    if (!validate(loaded, errorMessage)) {
        return false;
    }

    config = loaded;
    return true;
}
