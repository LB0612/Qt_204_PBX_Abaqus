#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QString>
#include "ProjectManager.h"

// class ProjectConfig;

class SimulationManager : public QObject
{
    Q_OBJECT
public:
    explicit SimulationManager(QObject *parent = nullptr);
    ~SimulationManager();

    static SimulationManager& instance();

    void startTask(const QString &projectPath, const ProjectConfig &config);
    void stopTask(const QString &projectPath);
    bool isRunning(const QString &projectPath);
    QString getProjectLog(const QString &projectPath);
    int getProjectProgress(const QString &projectPath);
    QString getProjectStatus(const QString &projectPath);

signals:
    void logReceived(const QString &projectPath, const QString &log);
    void progressUpdated(const QString &projectPath, int progress, const QString &status);
    void taskFinished(const QString &projectPath, int exitCode);

private slots:
    void handleProcessOutput();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void extractProgress(const QString &projectPath, const QString &output, double maxTime);
    double readMaxTimeFromDatFile(const QString &datFile);

private:
    QMap<QString, QProcess*> m_processes;
    QMap<QString, QString> m_executionLogs;
    QMap<QString, QString> m_logBuffers;
    QMap<QString, double> m_currentProgress;
    QMap<QString, QString> m_currentStatusText;
    QMap<QString, double> m_maxTimes;
    QMap<QString, ProjectConfig> m_projectConfigs;
};

#endif // SIMULATIONMANAGER_H