#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <QObject>
#include <QString>

#include "ProjectManager.h"

class SimulationManager : public QObject
{
    Q_OBJECT

public:
    explicit SimulationManager(QObject *parent = nullptr);
    ~SimulationManager();

    static SimulationManager &instance();

    void startTask(const QString &projectPath, const ProjectConfig &config);
    void stopTask(const QString &projectPath);
    bool isRunning(const QString &projectPath) const;

signals:
    void logReceived(const QString &projectPath, const QString &log);
    void progressUpdated(const QString &projectPath, int progress, const QString &status);
    void taskFinished(const QString &projectPath, int exitCode);
};

#endif
