#include "ExplosiveConfigManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace {
const QString FILE_EXPLOSIVE = QStringLiteral("explosive.json");
const int EXPLOSIVE_SCHEMA_VERSION = 1;

QString explosiveFilePath(const QString &projectPath)
{
    return QDir(projectPath).filePath(QStringLiteral("config/") + FILE_EXPLOSIVE);
}
}

bool ExplosiveConfigManager::validate(
    const ExplosiveConfig &config,
    QString &errorMessage)
{
    if (config.density <= 0.0) {
        errorMessage = QStringLiteral("炸药密度必须大于 0。");
        return false;
    }

    if (config.initialElasticModulus <= 0.0) {
        errorMessage = QStringLiteral("固化初始杨氏模量必须大于 0。");
        return false;
    }

    if (config.initialPoissonRatio < 0.0) {
        errorMessage = QStringLiteral("固化初始泊松比不能为负数。");
        return false;
    }

    if (config.finalElasticModulus <= 0.0) {
        errorMessage = QStringLiteral("固化结束杨氏模量必须大于 0。");
        return false;
    }

    if (config.finalPoissonRatio < 0.0) {
        errorMessage = QStringLiteral("固化结束泊松比不能为负数。");
        return false;
    }

    if (config.thermalConductivity <= 0.0) {
        errorMessage = QStringLiteral("炸药传导率必须大于 0。");
        return false;
    }

    if (config.yieldStress <= 0.0) {
        errorMessage = QStringLiteral("炸药屈服应力必须大于 0。");
        return false;
    }

    if (config.specificHeat <= 0.0) {
        errorMessage = QStringLiteral("炸药比热必须大于 0。");
        return false;
    }

    if (config.expansionCoefficient <= 0.0) {
        errorMessage = QStringLiteral("炸药膨胀系数必须大于 0。");
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

    QDir configDir(QDir(projectPath).filePath(QStringLiteral("config")));
    if (!configDir.exists() && !QDir(projectPath).mkpath(QStringLiteral("config"))) {
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

    QSaveFile file(explosiveFilePath(projectPath));
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
    QFile file(explosiveFilePath(projectPath));
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
