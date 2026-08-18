#include "SimulationManager.h"

SimulationManager::SimulationManager(QObject *parent)
    : QObject(parent)
{
}

SimulationManager::~SimulationManager() = default;

SimulationManager &SimulationManager::instance()
{
    static SimulationManager manager;
    return manager;
}

void SimulationManager::startTask(const QString &projectPath, const ProjectConfig &config)
{
    Q_UNUSED(projectPath);
    Q_UNUSED(config);
}

void SimulationManager::stopTask(const QString &projectPath)
{
    Q_UNUSED(projectPath);
}

bool SimulationManager::isRunning(const QString &projectPath) const
{
    Q_UNUSED(projectPath);
    return false;
}
