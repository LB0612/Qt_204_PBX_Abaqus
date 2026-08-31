#include "MoldConfigManager.h"

#include "ProjectPaths.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <cmath>

namespace {
const int MOLD_SCHEMA_VERSION = 1;

bool isPositiveFinite(double value)
{
    return std::isfinite(value) && value > 0.0;
}

bool isValidPoissonRatio(double value)
{
    return std::isfinite(value)
        && value > 0.0
        && value < 0.5;
}

} // namespace

bool MoldConfigManager::validate(
    const MoldConfig &config,
    QString &errorMessage)
{
    if (!isPositiveFinite(config.density)) {
        errorMessage = QStringLiteral("模具密度必须是有限数值且大于 0。");
        return false;
    }

    if (!isPositiveFinite(config.elasticModulus)) {
        errorMessage = QStringLiteral("模具弹性模量必须是有限数值且大于 0。");
        return false;
    }

    if (!isValidPoissonRatio(config.poissonRatio)) {
        errorMessage = QStringLiteral("模具泊松比必须是有限数值，且大于 0 并小于 0.5。");
        return false;
    }

    if (!isPositiveFinite(config.thermalConductivity)) {
        errorMessage = QStringLiteral("模具热导率必须是有限数值且大于 0。");
        return false;
    }

    if (!isPositiveFinite(config.specificHeat)) {
        errorMessage = QStringLiteral("模具比热容必须是有限数值且大于 0。");
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

    const QString configDirPath =
        ProjectPaths::configDirectoryPath(projectPath);
    if (!QDir(configDirPath).exists()
        && !QDir().mkpath(configDirPath)) {
        return false;
    }

    QJsonObject jsonObj;
    jsonObj[QStringLiteral("schemaVersion")] = MOLD_SCHEMA_VERSION;
    jsonObj[QStringLiteral("density")] = config.density;
    jsonObj[QStringLiteral("elasticModulus")] = config.elasticModulus;
    jsonObj[QStringLiteral("poissonRatio")] = config.poissonRatio;
    jsonObj[QStringLiteral("thermalConductivity")] = config.thermalConductivity;
    jsonObj[QStringLiteral("specificHeat")] = config.specificHeat;

    QSaveFile file(ProjectPaths::moldConfigPath(projectPath));
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
    QFile file(ProjectPaths::moldConfigPath(projectPath));
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
