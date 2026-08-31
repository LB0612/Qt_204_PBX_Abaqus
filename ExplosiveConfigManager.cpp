#include "ExplosiveConfigManager.h"

#include "ProjectPaths.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <cmath>

namespace {
const int EXPLOSIVE_SCHEMA_VERSION = 1;

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

bool ExplosiveConfigManager::validate(
    const ExplosiveConfig &config,
    QString &errorMessage)
{
    if (!isPositiveFinite(config.density)) {
        errorMessage = QStringLiteral("炸药密度必须是有限数值且大于 0。");
        return false;
    }

    if (!isPositiveFinite(config.initialElasticModulus)) {
        errorMessage = QStringLiteral("固化初始杨氏模量必须是有限数值且大于 0。");
        return false;
    }

    if (!isValidPoissonRatio(config.initialPoissonRatio)) {
        errorMessage = QStringLiteral("固化初始泊松比必须是有限数值，且大于 0 并小于 0.5。");
        return false;
    }

    if (!isPositiveFinite(config.finalElasticModulus)) {
        errorMessage = QStringLiteral("固化结束杨氏模量必须是有限数值且大于 0。");
        return false;
    }

    if (!isValidPoissonRatio(config.finalPoissonRatio)) {
        errorMessage = QStringLiteral("固化结束泊松比必须是有限数值，且大于 0 并小于 0.5。");
        return false;
    }

    if (!isPositiveFinite(config.thermalConductivity)) {
        errorMessage = QStringLiteral("炸药传导率必须是有限数值且大于 0。");
        return false;
    }

    if (!isPositiveFinite(config.yieldStress)) {
        errorMessage = QStringLiteral("炸药屈服应力必须是有限数值且大于 0。");
        return false;
    }

    if (!isPositiveFinite(config.specificHeat)) {
        errorMessage = QStringLiteral("炸药比热必须是有限数值且大于 0。");
        return false;
    }

    if (!isPositiveFinite(config.expansionCoefficient)) {
        errorMessage = QStringLiteral("炸药膨胀系数必须是有限数值且大于 0。");
        return false;
    }

    return true;
}

bool ExplosiveConfigManager::save(
    const QString &projectPath,
    const ExplosiveConfig &config)
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
    jsonObj[QStringLiteral("schemaVersion")] = EXPLOSIVE_SCHEMA_VERSION;
    jsonObj[QStringLiteral("density")] = config.density;
    jsonObj[QStringLiteral("initialElasticModulus")] = config.initialElasticModulus;
    jsonObj[QStringLiteral("initialPoissonRatio")] = config.initialPoissonRatio;
    jsonObj[QStringLiteral("finalElasticModulus")] = config.finalElasticModulus;
    jsonObj[QStringLiteral("finalPoissonRatio")] = config.finalPoissonRatio;
    jsonObj[QStringLiteral("thermalConductivity")] = config.thermalConductivity;
    jsonObj[QStringLiteral("yieldStress")] = config.yieldStress;
    jsonObj[QStringLiteral("specificHeat")] = config.specificHeat;
    jsonObj[QStringLiteral("expansionCoefficient")] = config.expansionCoefficient;

    QSaveFile file(ProjectPaths::explosiveConfigPath(projectPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool ExplosiveConfigManager::load(
    const QString &projectPath,
    ExplosiveConfig &config)
{
    QFile file(ProjectPaths::explosiveConfigPath(projectPath));
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
    if (schemaVersion != EXPLOSIVE_SCHEMA_VERSION) {
        return false;
    }

    const QStringList requiredKeys = {
        QStringLiteral("density"),
        QStringLiteral("initialElasticModulus"),
        QStringLiteral("initialPoissonRatio"),
        QStringLiteral("finalElasticModulus"),
        QStringLiteral("finalPoissonRatio"),
        QStringLiteral("thermalConductivity"),
        QStringLiteral("yieldStress"),
        QStringLiteral("specificHeat"),
        QStringLiteral("expansionCoefficient")
    };

    for (const QString &key : requiredKeys) {
        if (!jsonObj.contains(key)) {
            return false;
        }
    }

    ExplosiveConfig loaded;
    loaded.schemaVersion = schemaVersion;
    loaded.density = jsonObj.value(QStringLiteral("density")).toDouble();
    loaded.initialElasticModulus = jsonObj.value(QStringLiteral("initialElasticModulus")).toDouble();
    loaded.initialPoissonRatio = jsonObj.value(QStringLiteral("initialPoissonRatio")).toDouble();
    loaded.finalElasticModulus = jsonObj.value(QStringLiteral("finalElasticModulus")).toDouble();
    loaded.finalPoissonRatio = jsonObj.value(QStringLiteral("finalPoissonRatio")).toDouble();
    loaded.thermalConductivity = jsonObj.value(QStringLiteral("thermalConductivity")).toDouble();
    loaded.yieldStress = jsonObj.value(QStringLiteral("yieldStress")).toDouble();
    loaded.specificHeat = jsonObj.value(QStringLiteral("specificHeat")).toDouble();
    loaded.expansionCoefficient = jsonObj.value(QStringLiteral("expansionCoefficient")).toDouble();

    QString errorMessage;
    if (!validate(loaded, errorMessage)) {
        return false;
    }

    config = loaded;
    return true;
}
