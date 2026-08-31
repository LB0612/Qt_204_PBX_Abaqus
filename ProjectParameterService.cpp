#include "ProjectParameterService.h"

#include "BoundaryConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "ProjectPaths.h"
#include "SimulationConfigManager.h"
#include "StructureConfigManager.h"

#include <QFileInfo>

QString ProjectParameterService::structureConfigPath(
    const QString &projectPath)
{
    return ProjectPaths::structureConfigPath(projectPath);
}

QString ProjectParameterService::explosiveConfigPath(
    const QString &projectPath)
{
    return ProjectPaths::explosiveConfigPath(projectPath);
}

QString ProjectParameterService::moldConfigPath(
    const QString &projectPath)
{
    return ProjectPaths::moldConfigPath(projectPath);
}

QString ProjectParameterService::boundaryConfigPath(
    const QString &projectPath)
{
    return ProjectPaths::boundaryConfigPath(projectPath);
}

QString ProjectParameterService::simulationConfigPath(
    const QString &projectPath)
{
    return ProjectPaths::simulationConfigPath(projectPath);
}

bool ProjectParameterService::initializeDefaults(
    const QString &projectPath,
    QString &errorMessage)
{
    if (!StructureConfigManager::save(
            projectPath,
            StructureConfig())) {
        errorMessage = QStringLiteral(
            "默认结构参数初始化失败。"
        );
        return false;
    }

    if (!ExplosiveConfigManager::save(
            projectPath,
            ExplosiveConfig())) {
        errorMessage = QStringLiteral(
            "默认炸药参数初始化失败。"
        );
        return false;
    }

    if (!MoldConfigManager::save(
            projectPath,
            MoldConfig())) {
        errorMessage = QStringLiteral(
            "默认模具参数初始化失败。"
        );
        return false;
    }

    if (!BoundaryConfigManager::save(
            projectPath,
            BoundaryConfig())) {
        errorMessage = QStringLiteral(
            "默认边界条件初始化失败。"
        );
        return false;
    }

    if (!SimulationConfigManager::save(
            projectPath,
            SimulationConfig())) {
        errorMessage = QStringLiteral(
            "默认仿真设置初始化失败。"
        );
        return false;
    }

    errorMessage.clear();
    return true;
}

bool ProjectParameterService::loadAll(
    const QString &projectPath,
    ProjectParameters &parameters,
    QString &errorMessage)
{
    parameters = ProjectParameters();

    if (!StructureConfigManager::load(projectPath, parameters.structure)) {
        errorMessage = QStringLiteral("请先填写并保存结构参数。");
        return false;
    }

    if (!ExplosiveConfigManager::load(projectPath, parameters.explosive)) {
        errorMessage = QStringLiteral("请先填写并保存炸药参数。");
        return false;
    }

    if (!MoldConfigManager::load(projectPath, parameters.mold)) {
        errorMessage = QStringLiteral("请先填写并保存模具参数。");
        return false;
    }

    if (!BoundaryConfigManager::load(projectPath, parameters.boundary)) {
        errorMessage = QStringLiteral("请先填写并保存边界条件。");
        return false;
    }

    if (!SimulationConfigManager::load(projectPath, parameters.simulation)) {
        errorMessage = QStringLiteral("请先填写并保存仿真设置。");
        return false;
    }

    errorMessage.clear();
    return true;
}

bool ProjectParameterService::loadAvailable(
    const QString &projectPath,
    ProjectParameters &parameters,
    QString &errorMessage)
{
    parameters = ProjectParameters();

    if (QFileInfo::exists(structureConfigPath(projectPath))
        && !StructureConfigManager::load(projectPath, parameters.structure)) {
        errorMessage = QStringLiteral("结构参数配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(explosiveConfigPath(projectPath))
        && !ExplosiveConfigManager::load(projectPath, parameters.explosive)) {
        errorMessage = QStringLiteral("炸药参数配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(moldConfigPath(projectPath))
        && !MoldConfigManager::load(projectPath, parameters.mold)) {
        errorMessage = QStringLiteral("模具参数配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(boundaryConfigPath(projectPath))
        && !BoundaryConfigManager::load(projectPath, parameters.boundary)) {
        errorMessage = QStringLiteral("边界条件配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(simulationConfigPath(projectPath))
        && !SimulationConfigManager::load(
            projectPath,
            parameters.simulation)) {
        errorMessage = QStringLiteral("仿真设置配置文件无效或无法读取。");
        return false;
    }

    errorMessage.clear();
    return true;
}
