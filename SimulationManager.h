#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <QByteArray>
#include <QObject>
#include <QString>

class QProcess;
class QTimer;

enum class SimulationState
{
    Idle,
    T0Running,
    T1Running,
    Stopping,
    Stopped,
    Finished,
    Failed
};

class SimulationManager : public QObject
{
    Q_OBJECT

public:
    explicit SimulationManager(QObject *parent = nullptr);
    ~SimulationManager() override;

    SimulationState state() const;
    QString projectPath() const;
    QString jobName() const;

    bool isActive() const;

    void setProjectContext(const QString &projectPath, const QString &abaqusPath);

    bool checkReady(QString &errorMessage) const;
    bool hasLockFiles() const;
    bool hasValidPreviousResult(QString &message);

    QString currentJobLockPath() const;
    bool clearCurrentJobLock(QString &errorMessage) const;

    void startTask(const QString &projectPath, const QString &abaqusPath);
    void stopTask();

    void continueLockWait();
    void respondToForceKillPrompt(bool continueWaiting);
    void forceCloseTrackedProcesses();

signals:
    void stateChanged(SimulationState state);
    void statusChanged(const QString &text);
    void phaseChanged(const QString &text);
    void jobChanged(const QString &jobName);
    void progressUpdated(int value);
    void logReceived(const QString &text);
    void monitorResetRequested();

    void simulationFinished();
    void simulationStopped();
    void errorOccurred(const QString &title, const QString &text);
    void forceKillRequested();

private:
    void startTaskInternal();
    void setSimulationState(SimulationState state);
    void clearRunningSimulationContext();

    void handleTerminateRequestFailure(const QString &reason);
    void sendAbaqusTerminateCommand();
    void waitForJobLockRelease(const QString &lockPath);
    void onAbaqusJobTerminateFinished();

    void closeAbaqusProcesses();
    bool isCurrentJobLockPresent() const;
    void finishStopState();

    void updateAbaqusLog();
    void readAbaqusLogFile(
        const QString &path,
        const QString &tag,
        qint64 &readOffset,
        QByteArray &pendingData
    );
    void updateProgressFromStaLine(const QString &line);
    double loadSimulationTotalTime();

    QString calculateInputFingerprint() const;
    bool saveRunFingerprint(const QString &filePath) const;
    bool fingerprintsMatch() const;
    bool fingerprintMatchesStored(const QString &storedPath) const;
    bool recoverSuccessFingerprintIfPossible();

    QString m_projectPath;
    QString m_abaqusPath;

    QProcess *abaqusProcess = nullptr;
    QTimer *simulationTimer = nullptr;

    QString simulationMsgPath;
    QString simulationStaPath;
    QString simulationDatPath;
    qint64 simulationMsgReadOffset = 0;
    qint64 simulationStaReadOffset = 0;
    QByteArray simulationMsgPending;
    QByteArray simulationStaPending;
    double simulationTotalTime = 0.0;

    bool simulationUserStopped = false;
    SimulationState simulationState = SimulationState::Idle;

    QString currentJobName;
    QString runningProjectPath;
    QString runningAbaqusPath;

    QTimer *m_lockWaitTimer = nullptr;
    int m_lockWaitTries = 0;
    QString m_lockWaitPath;
};

#endif
