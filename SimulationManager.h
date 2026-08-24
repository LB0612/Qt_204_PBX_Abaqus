#ifndef SIMULATIONMANAGER_H
#define SIMULATIONMANAGER_H

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
class QProcessEnvironment;
class QTimer;

enum class SimulationState
{
    Idle,
    T0Running,
    T1Running,
    T2Running,
    Stopping,
    Stopped,
    Finished,
    Failed,
    PostProcessFailed
};

enum class SimulationResumeMode
{
    FullRun,
    PostProcessOnly,
    AlreadyComplete
};

class SimulationManager : public QObject
{
    Q_OBJECT

public:
    explicit SimulationManager(QObject *parent = nullptr);
    ~SimulationManager() override;

    SimulationState state() const;
    QString projectPath() const;

    bool isActive() const;

    void setProjectContext(const QString &projectPath, const QString &abaqusPath);
    void setForceFullRerun(bool forceFullRerun);

    bool checkReady(QString &errorMessage) const;
    bool hasLockFiles() const;

    SimulationResumeMode detectResumeMode() const;
    bool hasValidPreviousResult(QString &message);
    QString resumeModeMessage(SimulationResumeMode mode) const;

    QString currentJobLockPath() const;
    bool clearCurrentJobLock(QString &errorMessage) const;

    void startTask(const QString &projectPath, const QString &abaqusPath);
    void stopTask();

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
    void errorOccurred(const QString &title, const QString &text);
    void forceKillRequested();

private:
    void startTaskInternal();
    void setSimulationState(SimulationState state);

    void prepareRunContext();
    void clearRunArtifactsForFullRun();
    void appendProcessLog(
        const QString &logPath,
        const QByteArray &data,
        bool isError
    );

    void startT0Stage();
    void handleT0Finished(int exitCode, QProcess::ExitStatus exitStatus);

    void startT1Stage();
    void handleT1Finished(int exitCode, QProcess::ExitStatus exitStatus);

    void startT2Stage();
    void handleT2Finished(int exitCode, QProcess::ExitStatus exitStatus);

    void finishSimulationSuccess();
    void failSolverStage(const QString &statusText, const QString &logText, const QString &errorText);
    void failPostProcessStage(const QString &statusText, const QString &logText, const QString &errorText);

    void clearRunningSimulationContext(bool removeRunningFingerprint = true);
    void clearRunningPostContext(bool removeRunningPostFingerprint = true);

    void handleTerminateRequestFailure(const QString &reason);
    void sendAbaqusTerminateCommand();
    void waitForJobLockRelease(const QString &lockPath);
    void onAbaqusJobTerminateFinished();
    void continueLockWait();

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
    QString calculatePostFingerprint() const;
    bool saveRunFingerprint(const QString &filePath, const QString &fingerprint) const;
    bool fingerprintsMatch() const;
    bool postFingerprintsMatch() const;
    bool fingerprintMatchesStored(
        const QString &storedPath,
        const QString &currentFingerprint
    ) const;
    bool recoverSuccessFingerprintIfPossible();
    bool recoverSuccessPostFingerprintIfPossible();
    bool promoteRunningInputFingerprint(const QString &projectPath) const;
    bool promoteRunningPostFingerprint(const QString &projectPath) const;

    bool hasValidSolverResult() const;
    bool hasCompletePostProcess() const;
    bool readSuccessFlag(const QString &flagPath) const;

    QProcessEnvironment buildT2ProcessEnvironment() const;

    QString activeProjectPath() const;
    QString activeJobLockPath() const;

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

    QString t0LogPath;
    QString t1LogPath;
    QString t2LogPath;

    bool simulationUserStopped = false;
    bool m_forceFullRerun = false;
    SimulationState simulationState = SimulationState::Idle;

    QString currentJobName;
    QString runningProjectPath;
    QString runningAbaqusPath;

    QTimer *m_lockWaitTimer = nullptr;
    int m_lockWaitTries = 0;
    QString m_lockWaitPath;
};

#endif
