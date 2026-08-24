#include "SimulationManager.h"

#include "StructureConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "BoundaryConfigManager.h"
#include "SimulationConfigManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QCryptographicHash>

namespace {

QString runningInputFingerprintPath(const QString &projectDir)
{
    return QDir(QDir(projectDir).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("running_input.sha256"));
}

QString lastSuccessInputFingerprintPath(const QString &projectDir)
{
    return QDir(QDir(projectDir).filePath(QStringLiteral("abaqus")))
        .filePath(QStringLiteral("last_success_input.sha256"));
}

void removeRunningInputFingerprint(const QString &projectDir)
{
    if (projectDir.isEmpty()) {
        return;
    }

    QFile::remove(runningInputFingerprintPath(projectDir));
}

bool promoteRunningInputFingerprint(const QString &projectDir)
{
    const QString runningPath = runningInputFingerprintPath(projectDir);
    const QString lastPath = lastSuccessInputFingerprintPath(projectDir);

    if (!QFile::exists(runningPath)) {
        return false;
    }

    QFile::remove(lastPath);

    if (QFile::rename(runningPath, lastPath)) {
        return true;
    }

    if (QFile::copy(runningPath, lastPath)) {
        QFile::remove(runningPath);
        return true;
    }

    return false;
}

} // namespace


SimulationManager::SimulationManager(QObject *parent)
    : QObject(parent)
{
}

SimulationManager::~SimulationManager()
{
    if (simulationTimer) {
        simulationTimer->stop();
    }
    if (abaqusProcess && abaqusProcess->state() != QProcess::NotRunning) {
        abaqusProcess->kill();
        abaqusProcess->waitForFinished(3000);
    }
}

SimulationState SimulationManager::state() const
{
    return simulationState;
}

QString SimulationManager::projectPath() const
{
    return m_projectPath;
}

QString SimulationManager::jobName() const
{
    return currentJobName;
}

void SimulationManager::setProjectContext(const QString &projectPath, const QString &abaqusPath)
{
    m_projectPath = projectPath;
    m_abaqusPath = abaqusPath;
}

void SimulationManager::startTask(const QString &projectPath, const QString &abaqusPath)
{
    m_projectPath = projectPath;
    m_abaqusPath = abaqusPath;
    startTaskInternal();
}

void SimulationManager::continueLockWait()
{
    if (m_lockWaitTimer) {
        m_lockWaitTries = 0;
        m_lockWaitTimer->start();
    }
}

void SimulationManager::respondToForceKillPrompt(bool continueWaiting)
{
    if (!m_lockWaitPath.isEmpty() && !QFile::exists(m_lockWaitPath)) {
        if (m_lockWaitTimer) {
            m_lockWaitTimer->deleteLater();
            m_lockWaitTimer = nullptr;
        }
        m_lockWaitTries = 0;
        m_lockWaitPath.clear();
        onAbaqusJobTerminateFinished();
        return;
    }

    if (continueWaiting) {
        continueLockWait();
        return;
    }

    if (m_lockWaitTimer) {
        m_lockWaitTimer->deleteLater();
        m_lockWaitTimer = nullptr;
    }
    m_lockWaitTries = 0;
    m_lockWaitPath.clear();

    forceCloseTrackedProcesses();
}

bool SimulationManager::isActive() const
{
    return simulationState == SimulationState::T0Running
        || simulationState == SimulationState::T1Running
        || simulationState == SimulationState::Stopping;
}

void SimulationManager::setSimulationState(SimulationState state)
{
    if (simulationState == state) {
        return;
    }
    simulationState = state;
    emit stateChanged(simulationState);
}

void SimulationManager::clearRunningSimulationContext()
{
    const QString projectPath =
        runningProjectPath.isEmpty()
            ? m_projectPath
            : runningProjectPath;
    removeRunningInputFingerprint(projectPath);

    currentJobName.clear();
    runningProjectPath.clear();
    runningAbaqusPath.clear();

    simulationMsgPath.clear();
    simulationStaPath.clear();
    simulationDatPath.clear();

    simulationMsgReadOffset = 0;
    simulationStaReadOffset = 0;
    simulationMsgPending.clear();
    simulationStaPending.clear();
    simulationTotalTime = 0.0;
}

void SimulationManager::handleTerminateRequestFailure(const QString &reason)
{
    emit logReceived(
        QStringLiteral("[ERROR] ") + reason
    );

    const QString projectPath =
        runningProjectPath.isEmpty()
            ? m_projectPath
            : runningProjectPath;

    const QString lockPath =
        QDir(projectPath).filePath(
            QStringLiteral("abaqus/")
            + currentJobName
            + QStringLiteral(".lck")
        );

    const bool processEnded =
        !abaqusProcess
        || abaqusProcess->state() == QProcess::NotRunning;

    const bool lockGone =
        currentJobName.isEmpty()
        || !QFile::exists(lockPath);

    if (processEnded) {
        if (lockGone) {
            emit logReceived(
                QStringLiteral(
                    "[SYS] 主仿真进程已结束且锁文件不存在，"
                    "继续终止收尾"
                )
            );

            onAbaqusJobTerminateFinished();
            return;
        }

        emit statusChanged(
            QStringLiteral("等待Abaqus释放Job锁")
        );

        emit logReceived(
            QStringLiteral(
                "[SYS] 主进程已结束，"
                "但Job锁文件仍存在，继续等待释放"
            )
        );

        waitForJobLockRelease(lockPath);
        return;
    }

    simulationUserStopped = false;

    setSimulationState(SimulationState::T1Running);

    emit statusChanged(
        QStringLiteral("终止请求失败，仿真仍在运行")
    );

    emit logReceived(
        QStringLiteral("[ERROR] Abaqus Job 可能仍在运行")
    );
}

QString SimulationManager::calculateInputFingerprint() const
{
    if (m_projectPath.isEmpty()) {
        return QString();
    }

    static const QStringList relativePaths = {
        QStringLiteral("config/structure.json"),
        QStringLiteral("config/explosive.json"),
        QStringLiteral("config/mold.json"),
        QStringLiteral("config/boundary.json"),
        QStringLiteral("config/simulation.json"),
        QStringLiteral("abaqus/t0.py"),
        QStringLiteral("abaqus/t1.py"),
        QStringLiteral("abaqus/335K.for"),
    };

    QCryptographicHash hash(QCryptographicHash::Sha256);
    const QDir projectDir(m_projectPath);

    for (const QString &relativePath : relativePaths) {
        QFile file(projectDir.filePath(relativePath));
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }

        hash.addData(relativePath.toUtf8());
        hash.addData(file.readAll());
    }

    return QString::fromLatin1(hash.result().toHex());
}

bool SimulationManager::saveRunFingerprint(const QString &filePath) const
{
    const QString fingerprint = calculateInputFingerprint();
    if (fingerprint.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    file.write(fingerprint.toLatin1());
    file.write("\n");
    return true;
}

bool SimulationManager::fingerprintsMatch() const
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    const QString lastPath = lastSuccessInputFingerprintPath(m_projectPath);
    QFile file(lastPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QString stored = QString::fromLatin1(file.readAll()).trimmed();
    file.close();

    if (stored.isEmpty()) {
        return false;
    }

    const QString current = calculateInputFingerprint();
    if (current.isEmpty()) {
        return false;
    }

    return stored == current;
}

bool SimulationManager::hasValidPreviousResult(
    QString &message) const
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    const QString projectDir = m_projectPath;

    const QString abaqusDir =
        QDir(projectDir).filePath(QStringLiteral("abaqus"));

    const QString jobName =
        QDir(projectDir).dirName() + QStringLiteral("_Job");

    const QString flagPath =
        QDir(abaqusDir).filePath(QStringLiteral("t1_finished.flag"));

    const QString odbPath =
        QDir(abaqusDir).filePath(jobName + QStringLiteral(".odb"));

    const QString lockPath =
        QDir(abaqusDir).filePath(jobName + QStringLiteral(".lck"));

    if (!QFile::exists(flagPath)) {
        return false;
    }

    if (!QFile::exists(odbPath)) {
        return false;
    }

    if (QFile::exists(lockPath)) {
        return false;
    }

    QFile flagFile(flagPath);
    if (!flagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QString flagContent =
        QString::fromUtf8(flagFile.readAll()).trimmed();
    flagFile.close();

    if (flagContent != QStringLiteral("success")) {
        return false;
    }

    if (!fingerprintsMatch()) {
        return false;
    }

    const QFileInfo flagInfo(flagPath);
    const QDateTime completedTime = flagInfo.lastModified();

    message = QStringLiteral(
        "检测到当前工程上一次仿真已经正常完成，"
        "且当前计算输入与上一次完全一致。\n\n"
        "完成时间：%1\n"
        "结果文件：%2\n\n"
        "通常不需要重复计算。"
    ).arg(
        completedTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
        odbPath
    );

    return true;
}

bool SimulationManager::checkReady(QString &errorMessage) const
{
    const QString projectDir = m_projectPath;
    const QString abaqusDir =
        QDir(projectDir).filePath(QStringLiteral("abaqus"));

    const QString t0Path =
        QDir(abaqusDir).filePath(QStringLiteral("t0.py"));

    const QStringList files = {
        t0Path,
        QDir(abaqusDir).filePath(QStringLiteral("t1.py")),
        QDir(abaqusDir).filePath(QStringLiteral("335K.for"))
    };

    for (const QString &file : files) {
        if (!QFile::exists(file)) {
            errorMessage =
                QStringLiteral("缺少文件:\n%1").arg(file);
            return false;
        }
    }

    const QString generationFlagPath =
        QDir(abaqusDir).filePath(
            QStringLiteral("generation_complete.flag")
        );

    if (!QFile::exists(generationFlagPath)) {
        errorMessage = QStringLiteral(
            "Abaqus 文件尚未完整生成，"
            "请重新点击“生成文件”。"
        );
        return false;
    }

    StructureConfig structure;
    if (!StructureConfigManager::load(projectDir, structure)) {
        errorMessage = QStringLiteral("请先填写并保存结构参数。");
        return false;
    }

    ExplosiveConfig explosive;
    if (!ExplosiveConfigManager::load(projectDir, explosive)) {
        errorMessage = QStringLiteral("请先填写并保存炸药参数。");
        return false;
    }

    MoldConfig mold;
    if (!MoldConfigManager::load(projectDir, mold)) {
        errorMessage = QStringLiteral("请先填写并保存模具参数。");
        return false;
    }

    BoundaryConfig boundary;
    if (!BoundaryConfigManager::load(projectDir, boundary)) {
        errorMessage = QStringLiteral("请先填写并保存边界条件。");
        return false;
    }

    SimulationConfig simulation;
    if (!SimulationConfigManager::load(projectDir, simulation)) {
        errorMessage = QStringLiteral("请先填写并保存仿真设置。");
        return false;
    }

    const QDateTime generationTime =
        QFileInfo(generationFlagPath).lastModified();

    const QStringList configFiles = {
        QDir(projectDir).filePath(QStringLiteral("config/structure.json")),
        QDir(projectDir).filePath(QStringLiteral("config/explosive.json")),
        QDir(projectDir).filePath(QStringLiteral("config/mold.json")),
        QDir(projectDir).filePath(QStringLiteral("config/boundary.json")),
        QDir(projectDir).filePath(QStringLiteral("config/simulation.json"))
    };

    for (const QString &configFile : configFiles) {
        const QFileInfo info(configFile);

        if (!info.exists()) {
            errorMessage = QStringLiteral(
                "参数配置文件缺失，"
                "请重新保存参数。"
            );
            return false;
        }

        if (info.lastModified() > generationTime) {
            errorMessage = QStringLiteral(
                "参数在 Abaqus 文件生成后"
                "发生过修改，"
                "请重新生成文件后再开始仿真。"
            );
            return false;
        }
    }

    for (const QString &filePath : files) {
        const QFileInfo info(filePath);

        if (!info.exists() || info.size() <= 0) {
            errorMessage =
                QStringLiteral("Abaqus 生成文件无效：\n%1")
                    .arg(filePath);
            return false;
        }

        if (info.lastModified() > generationTime) {
            errorMessage = QStringLiteral(
                "Abaqus 文件在完整生成后"
                "又发生过修改，"
                "请重新生成文件。"
            );
            return false;
        }
    }

    const QString abaqusPath = m_abaqusPath;
    if (abaqusPath.isEmpty() || !QFile::exists(abaqusPath)) {
        errorMessage = QStringLiteral("Abaqus路径无效");
        return false;
    }

    return true;
}

void SimulationManager::startTaskInternal()
{
    if (m_projectPath.isEmpty()) {
        return;
    }

    if (isActive()) {
        return;
    }

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        return;
    }

    if (hasLockFiles()) {
        emit errorOccurred(QStringLiteral("提示"), QStringLiteral("上一次仿真正在清理，请稍候。"));
        return;
    }

    QString error;
    if (!checkReady(error)) {
        emit errorOccurred(QStringLiteral("仿真无法启动"), error);
        return;
    }

    // m_projectPath 已是工程根目录，例如 D:/Test01
    const QString projectDir = m_projectPath;
    const QString abaqusDir =
        QDir(projectDir).filePath(QStringLiteral("abaqus"));

    if (!saveRunFingerprint(runningInputFingerprintPath(projectDir))) {
        emit errorOccurred(
            QStringLiteral("仿真无法启动"),
            QStringLiteral("无法记录本次仿真输入指纹。")
        );
        return;
    }

    const QString t0Path =
        QDir(abaqusDir).filePath(QStringLiteral("t0.py"));
    const QString t1Path =
        QDir(abaqusDir).filePath(QStringLiteral("t1.py"));

    const QString abaqusPath = m_abaqusPath;

    const QString logsDir =
        QDir(projectDir).filePath(QStringLiteral("logs"));
    QDir().mkpath(logsDir);
    const QString t0LogPath =
        QDir(logsDir).filePath(QStringLiteral("t0.log"));
    const QString t1LogPath =
        QDir(logsDir).filePath(QStringLiteral("t1.log"));

    auto clearLogFile = [](const QString &logPath) {
        QFile logFile(logPath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            logFile.close();
        }
    };
    auto appendProcessLog =
        [this](const QString &logPath, const QByteArray &data, bool isError) {
            if (data.isEmpty()) {
                return;
            }
            QFile logFile(logPath);
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                logFile.write(data);
            }
            const QString text = QString::fromLocal8Bit(data);
            if (text.contains(QStringLiteral("License Manager"))
                || text.contains(QStringLiteral("checked out"))) {
                emit logReceived(
                    QStringLiteral("[ABAQUS] ") + text
                );
            } else if (isError) {
                emit logReceived(
                    QStringLiteral("[ERROR] ") + text
                );
            } else {
                emit logReceived(
                    QStringLiteral("[SYS] ") + text
                );
            }
        };

    clearLogFile(t0LogPath);
    clearLogFile(t1LogPath);

    QFile::remove(
        QDir(abaqusDir).filePath(QStringLiteral("t0_finished.flag"))
    );
    QFile::remove(
        QDir(abaqusDir).filePath(QStringLiteral("t1_finished.flag"))
    );
    QFile::remove(
        QDir(abaqusDir).filePath(QStringLiteral("stop.flag"))
    );

    simulationUserStopped = false;

    runningProjectPath = projectDir;
    runningAbaqusPath = abaqusPath;

    currentJobName.clear();

    setSimulationState(SimulationState::T0Running);

    emit monitorResetRequested();
    emit progressUpdated(0);
    emit jobChanged(QString());
    emit statusChanged(
        QStringLiteral("正在启动 Abaqus...")
    );
    emit phaseChanged(
        QStringLiteral("阶段: 模型建立(t0)")
    );
    emit logReceived(
        QStringLiteral("[SYS] 开始仿真")
    );

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    // ---------- 阶段 1：t0.py ----------
    QProcess *t0Process = new QProcess(this);
    abaqusProcess = t0Process;
    t0Process->setWorkingDirectory(abaqusDir);

    connect(
        t0Process,
        &QProcess::readyReadStandardOutput,
        this,
        [this, t0Process, t0LogPath, appendProcessLog]() {
            appendProcessLog(
                t0LogPath,
                t0Process->readAllStandardOutput(),
                false
            );
        }
    );

    connect(
        t0Process,
        &QProcess::readyReadStandardError,
        this,
        [this, t0Process, t0LogPath, appendProcessLog]() {
            appendProcessLog(
                t0LogPath,
                t0Process->readAllStandardError(),
                true
            );
        }
    );

    connect(
        t0Process,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        [this, t0Process, t1Path, abaqusPath, abaqusDir, projectDir, t1LogPath, appendProcessLog](
            int exitCode,
            QProcess::ExitStatus exitStatus
        ) {
            if (abaqusProcess == t0Process) {
                abaqusProcess = nullptr;
            }
            t0Process->deleteLater();

            if (simulationUserStopped
                || simulationState == SimulationState::Stopping
                || simulationState == SimulationState::Stopped) {
                return;
            }

            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                setSimulationState(SimulationState::Failed);
                clearRunningSimulationContext();
                emit statusChanged(
                    QStringLiteral("t0.py 执行失败")
                );
                emit logReceived(
                    QStringLiteral("[SYS] t0.py 执行失败")
                );
                emit errorOccurred(QStringLiteral("错误"), QStringLiteral("t0.py 执行失败。"));
                return;
            }

            const QString caePath =
                QDir(abaqusDir).filePath(QStringLiteral("guhua.cae"));
            if (!QFile::exists(caePath)) {
                setSimulationState(SimulationState::Failed);
                clearRunningSimulationContext();
                emit statusChanged(
                    QStringLiteral("未生成 guhua.cae")
                );
                emit logReceived(
                    QStringLiteral("[SYS] t0 执行完成，但未生成 guhua.cae")
                );
                emit errorOccurred(QStringLiteral("错误"), QStringLiteral("t0 执行完成，但未生成 guhua.cae。"));
                return;
            }

            const QString flagPath =
                QDir(abaqusDir).filePath(QStringLiteral("t0_finished.flag"));

            QFile flagFile(flagPath);
            if (!flagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                setSimulationState(SimulationState::Failed);
                clearRunningSimulationContext();
                emit statusChanged(
                    QStringLiteral("t0执行失败")
                );
                emit logReceived(
                    QStringLiteral("[SYS] t0未生成完成标志")
                );
                emit errorOccurred(QStringLiteral("错误"), QStringLiteral("t0 未正常完成（缺少完成标志）。"));
                return;
            }

            const QString flagContent =
                QString::fromUtf8(flagFile.readAll()).trimmed();
            flagFile.close();

            if (flagContent != QStringLiteral("success")) {
                setSimulationState(SimulationState::Failed);
                clearRunningSimulationContext();
                emit statusChanged(
                    QStringLiteral("t0执行失败")
                );
                emit logReceived(
                    QStringLiteral("[SYS] t0完成标志内容无效")
                );
                emit errorOccurred(QStringLiteral("错误"), QStringLiteral("t0 未正常完成（完成标志无效）。"));
                return;
            }

            emit logReceived(
                QStringLiteral("[SYS] 模型生成完成，启动 t1.py")
            );
            emit statusChanged(
                QStringLiteral("Abaqus 求解中")
            );
            emit phaseChanged(
                QStringLiteral("阶段: Abaqus求解(t1)")
            );
            setSimulationState(SimulationState::T1Running);

            // ---------- 阶段 2：t1.py（t0 成功后新建进程）----------
            abaqusProcess = new QProcess(this);
            abaqusProcess->setWorkingDirectory(abaqusDir);

            connect(
                abaqusProcess,
                &QProcess::readyReadStandardOutput,
                this,
                [this, t1LogPath, appendProcessLog]() {
                    if (abaqusProcess) {
                        appendProcessLog(
                            t1LogPath,
                            abaqusProcess->readAllStandardOutput(),
                            false
                        );
                    }
                }
            );

            connect(
                abaqusProcess,
                &QProcess::readyReadStandardError,
                this,
                [this, t1LogPath, appendProcessLog]() {
                    if (abaqusProcess) {
                        appendProcessLog(
                            t1LogPath,
                            abaqusProcess->readAllStandardError(),
                            true
                        );
                    }
                }
            );

            const QString jobName =
                QDir(projectDir).dirName() + QStringLiteral("_Job");
            currentJobName = jobName;
            emit jobChanged(jobName);
            simulationMsgPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".msg"));
            simulationStaPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".sta"));
            simulationDatPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".dat"));
            simulationTotalTime = loadSimulationTotalTime();
            simulationMsgReadOffset = 0;
            simulationStaReadOffset = 0;
            simulationMsgPending.clear();
            simulationStaPending.clear();

            QFile::remove(simulationMsgPath);
            QFile::remove(simulationStaPath);
            QFile::remove(simulationDatPath);

            const QString odbPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".odb"));
            const QString t1FlagPath =
                QDir(abaqusDir).filePath(QStringLiteral("t1_finished.flag"));

            connect(
                abaqusProcess,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                [this, odbPath, t1FlagPath](
                    int t1ExitCode,
                    QProcess::ExitStatus exitStatus
                ) {
                    if (abaqusProcess) {
                        abaqusProcess->deleteLater();
                        abaqusProcess = nullptr;
                    }

                    if (simulationTimer) {
                        simulationTimer->stop();
                    }

                    if (simulationUserStopped
                        || simulationState == SimulationState::Stopping
                        || simulationState == SimulationState::Stopped) {
                        return;
                    }

                    if (exitStatus != QProcess::NormalExit || t1ExitCode != 0) {
                        setSimulationState(SimulationState::Failed);
                        clearRunningSimulationContext();
                        emit statusChanged(
                            QStringLiteral("t1.py 执行失败")
                        );
                        emit logReceived(
                            QStringLiteral("[SYS] t1.py 执行失败")
                        );
                        emit errorOccurred(QStringLiteral("错误"), QStringLiteral("t1.py 执行失败。"));
                        return;
                    }

                    if (!QFile::exists(odbPath)) {
                        setSimulationState(SimulationState::Failed);
                        clearRunningSimulationContext();
                        emit statusChanged(
                            QStringLiteral("Abaqus 未生成计算结果")
                        );
                        emit logReceived(
                            QStringLiteral("[SYS] Abaqus 未生成计算结果 ODB")
                        );
                        emit errorOccurred(QStringLiteral("错误"), QStringLiteral(
                                "Abaqus 未生成计算结果。\n\n"
                                "可能原因：\n"
                                "1. 用户中断计算\n"
                                "2. Abaqus 求解失败\n"
                                "3. 用户子程序错误"
                            ));
                        return;
                    }

                    if (!QFile::exists(t1FlagPath)) {
                        setSimulationState(SimulationState::Failed);
                        clearRunningSimulationContext();
                        emit statusChanged(
                            QStringLiteral("t1未正常完成")
                        );
                        emit logReceived(
                            QStringLiteral("[SYS] t1未生成完成标志")
                        );
                        emit errorOccurred(QStringLiteral("错误"), QStringLiteral("t1 未正常完成（缺少完成标志）。"));
                        return;
                    }

                    QFile t1FlagFile(t1FlagPath);
                    if (!t1FlagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        setSimulationState(SimulationState::Failed);
                        clearRunningSimulationContext();
                        emit statusChanged(
                            QStringLiteral("t1未正常完成")
                        );
                        emit logReceived(
                            QStringLiteral("[SYS] t1完成标志无法读取")
                        );
                        emit errorOccurred(
                            QStringLiteral("错误"),
                            QStringLiteral("t1 未正常完成（无法读取完成标志）。")
                        );
                        return;
                    }

                    const QString t1FlagContent =
                        QString::fromUtf8(t1FlagFile.readAll()).trimmed();
                    t1FlagFile.close();

                    if (t1FlagContent != QStringLiteral("success")) {
                        setSimulationState(SimulationState::Failed);
                        clearRunningSimulationContext();
                        emit statusChanged(
                            QStringLiteral("t1未正常完成")
                        );
                        emit logReceived(
                            QStringLiteral("[SYS] t1完成标志内容无效")
                        );
                        emit errorOccurred(
                            QStringLiteral("错误"),
                            QStringLiteral("t1 未正常完成（完成标志无效）。")
                        );
                        return;
                    }

                    const QString finishedProjectPath =
                        runningProjectPath.isEmpty()
                            ? m_projectPath
                            : runningProjectPath;
                    promoteRunningInputFingerprint(finishedProjectPath);

                    setSimulationState(SimulationState::Finished);
                    clearRunningSimulationContext();
                    emit statusChanged(
                        QStringLiteral("固化仿真完成")
                    );
                    emit phaseChanged(
                        QStringLiteral("阶段: 完成")
                    );
                    emit progressUpdated(100);
                    emit logReceived(
                        QStringLiteral("[SYS] 固化仿真完成")
                    );
                    emit simulationFinished();
                }
            );

            emit logReceived(
                QStringLiteral("[SYS] 启动 t1.py")
            );
            abaqusProcess->start(
                abaqusPath,
                QStringList()
                    << QStringLiteral("cae")
                    << QStringLiteral("script=") + t1Path
            );

            if (!abaqusProcess->waitForStarted(10000)) {
                abaqusProcess->deleteLater();
                abaqusProcess = nullptr;
                if (simulationTimer) {
                    simulationTimer->stop();
                }
                setSimulationState(SimulationState::Failed);
                clearRunningSimulationContext();
                emit statusChanged(
                    QStringLiteral("无法启动 t1.py")
                );
                emit errorOccurred(QStringLiteral("错误"), QStringLiteral("无法启动 t1.py。"));
                return;
            }

            if (!simulationTimer) {
                simulationTimer = new QTimer(this);
                connect(
                    simulationTimer,
                    &QTimer::timeout,
                    this,
                    &SimulationManager::updateAbaqusLog
                );
            }
            simulationTimer->start(1000);
        }
    );

    emit logReceived(
        QStringLiteral("[SYS] 启动 t0.py")
    );
    abaqusProcess->start(
        abaqusPath,
        QStringList()
            << QStringLiteral("cae")
            << QStringLiteral("script=") + t0Path
    );

    if (!abaqusProcess->waitForStarted(10000)) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;

        setSimulationState(SimulationState::Failed);
        clearRunningSimulationContext();

        emit statusChanged(
            QStringLiteral("无法启动 t0.py")
        );

        emit errorOccurred(QStringLiteral("错误"), QStringLiteral("无法启动 t0.py。"));

        return;
    }

    emit statusChanged(
        QStringLiteral("正在执行 Abaqus")
    );
}

void SimulationManager::stopTask()
{
    if (simulationState != SimulationState::T0Running
        && simulationState != SimulationState::T1Running) {
        return;
    }

    simulationUserStopped = true;
    setSimulationState(SimulationState::Stopping);

    emit statusChanged(
        QStringLiteral("正在请求终止")
    );
    emit logReceived(
        QStringLiteral("[SYS] 用户请求终止仿真")
    );

    if (!currentJobName.isEmpty()) {
        sendAbaqusTerminateCommand();
        return;
    }

    // t0 阶段尚无 Job：请求 CAE 退出，由 finished → finishStopState
    emit logReceived(
        QStringLiteral("[SYS] 当前无 Job，正在请求模型建立进程退出")
    );
    closeAbaqusProcesses();
}

bool SimulationManager::hasLockFiles() const
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    const QDir abaqusDir(
        QDir(m_projectPath).filePath(QStringLiteral("abaqus"))
    );
    const QStringList locks =
        abaqusDir.entryList(
            QStringList() << QStringLiteral("*.lck"),
            QDir::Files
        );
    return !locks.isEmpty();
}

void SimulationManager::sendAbaqusTerminateCommand()
{
    const QString projectPath =
        runningProjectPath.isEmpty()
            ? m_projectPath
            : runningProjectPath;

    const QString abaqusPath =
        runningAbaqusPath.isEmpty()
            ? m_abaqusPath
            : runningAbaqusPath;

    if (abaqusPath.isEmpty() || !QFile::exists(abaqusPath)) {
        handleTerminateRequestFailure(
            QStringLiteral("Abaqus 路径无效，无法发送 terminate")
        );
        return;
    }

    const QString abaqusDir =
        QDir(projectPath).filePath(QStringLiteral("abaqus"));
    const QString jobName = currentJobName;

    emit logReceived(
        QStringLiteral("[SYS] 已发送终止请求")
    );
    emit logReceived(
        QStringLiteral("[ABAQUS] terminate job=%1").arg(jobName)
    );

    QProcess *terminateProcess = new QProcess(this);
    terminateProcess->setWorkingDirectory(abaqusDir);

    connect(
        terminateProcess,
        &QProcess::readyReadStandardOutput,
        this,
        [this, terminateProcess]() {
            const QString text =
                QString::fromLocal8Bit(terminateProcess->readAllStandardOutput());
            if (!text.trimmed().isEmpty()) {
                emit logReceived(
                    QStringLiteral("[ABAQUS] ") + text
                );
            }
        }
    );
    connect(
        terminateProcess,
        &QProcess::readyReadStandardError,
        this,
        [this, terminateProcess]() {
            const QString text =
                QString::fromLocal8Bit(terminateProcess->readAllStandardError());
            if (!text.trimmed().isEmpty()) {
                emit logReceived(
                    QStringLiteral("[ABAQUS] ") + text
                );
            }
        }
    );
    connect(
        terminateProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        [this, terminateProcess, jobName, projectPath](
            int exitCode,
            QProcess::ExitStatus exitStatus
        ) {
            terminateProcess->deleteLater();

            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                handleTerminateRequestFailure(
                    QStringLiteral("abaqus terminate 执行失败")
                );
                return;
            }

            const QString lockPath =
                QDir(projectPath)
                    .filePath(
                        QStringLiteral("abaqus/")
                        + jobName
                        + QStringLiteral(".lck")
                    );

            waitForJobLockRelease(lockPath);
        }
    );

    terminateProcess->start(
        abaqusPath,
        QStringList()
            << QStringLiteral("terminate")
            << QStringLiteral("job=") + jobName
    );

    if (!terminateProcess->waitForStarted(10000)) {
        terminateProcess->deleteLater();
        handleTerminateRequestFailure(
            QStringLiteral("无法启动 abaqus terminate")
        );
    }
}

void SimulationManager::waitForJobLockRelease(const QString &lockPath)
{
    if (lockPath.isEmpty() || !QFile::exists(lockPath)) {
        onAbaqusJobTerminateFinished();
        return;
    }

    emit logReceived(
        QStringLiteral("[SYS] 等待Abaqus Job结束")
    );
    emit logReceived(
        QStringLiteral("[SYS] 等待Standard结束")
    );

    QTimer *waitTimer = new QTimer(this);
    waitTimer->setInterval(1000);
    m_lockWaitTries = 0;
    connect(
        waitTimer,
        &QTimer::timeout,
        this,
        [this, waitTimer, lockPath]() {
            ++m_lockWaitTries;

            if (!QFile::exists(lockPath)) {
                waitTimer->stop();
                waitTimer->deleteLater();
                m_lockWaitTimer = nullptr;
                m_lockWaitTries = 0;
                m_lockWaitPath.clear();
                onAbaqusJobTerminateFinished();
                return;
            }

            if (m_lockWaitTries == 30) {
                emit logReceived(
                    QStringLiteral(
                        "[SYS] 终止等待已超过30秒，"
                        "Job锁文件仍存在，继续等待..."
                    )
                );
            }

            if (m_lockWaitTries == 120) {
                waitTimer->stop();
                m_lockWaitTimer = waitTimer;
                m_lockWaitPath = lockPath;
                emit forceKillRequested();
                return;
            }
        }
    );
    waitTimer->start();
}

void SimulationManager::onAbaqusJobTerminateFinished()
{
    if (simulationState != SimulationState::Stopping) {
        return;
    }

    emit logReceived(
        QStringLiteral("[SYS] Job已终止，等待3秒后关闭Abaqus CAE")
    );

    if (simulationTimer) {
        simulationTimer->stop();
    }

    // 给 Abaqus 释放模块的时间，避免立刻 taskkill 触发未知消息框
    QTimer::singleShot(
        3000,
        this,
        &SimulationManager::closeAbaqusProcesses
    );
}

void SimulationManager::closeAbaqusProcesses()
{
    if (simulationState != SimulationState::Stopping) {
        return;
    }

    emit logReceived(
        QStringLiteral("[SYS] 正在关闭本次 Abaqus 进程")
    );

    if (!abaqusProcess
        || abaqusProcess->state() == QProcess::NotRunning) {
        finishStopState();
        return;
    }

    const qint64 pid = abaqusProcess->processId();

    if (pid <= 0) {
        emit logReceived(
            QStringLiteral(
                "[ERROR] 无法取得当前 Abaqus 进程 PID"
            )
        );
        return;
    }

    emit logReceived(
        QStringLiteral(
            "[SYS] 正在结束本次 Abaqus 进程树 PID=%1"
        ).arg(pid)
    );

    const int result = QProcess::execute(
        QStringLiteral("taskkill"),
        QStringList()
            << QStringLiteral("/PID")
            << QString::number(pid)
            << QStringLiteral("/T")
            << QStringLiteral("/F")
    );

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        abaqusProcess->waitForFinished(5000);
    }

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        emit logReceived(
            QStringLiteral(
                "[ERROR] 当前 Abaqus 进程仍未退出"
            )
        );
        emit statusChanged(
            QStringLiteral("Abaqus关闭失败")
        );
        return;
    }

    if (result != 0) {
        emit logReceived(
            QStringLiteral(
                "[SYS] taskkill返回非0，但当前进程已退出"
            )
        );
    }

    finishStopState();
}

bool SimulationManager::isCurrentJobLockPresent() const
{
    if (currentJobName.isEmpty()) {
        return false;
    }

    const QString projectPath =
        runningProjectPath.isEmpty()
            ? m_projectPath
            : runningProjectPath;

    if (projectPath.isEmpty()) {
        return false;
    }

    const QString lockPath =
        QDir(projectPath).filePath(
            QStringLiteral("abaqus/")
            + currentJobName
            + QStringLiteral(".lck")
        );

    return QFile::exists(lockPath);
}

void SimulationManager::forceCloseTrackedProcesses()
{
    if (simulationState != SimulationState::Stopping) {
        return;
    }

    emit logReceived(
        QStringLiteral(
            "[SYS] 用户选择强制结束本次 Abaqus 进程"
        )
    );

    bool killedTrackedProcessTree = false;

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {

        const qint64 pid = abaqusProcess->processId();

        if (pid <= 0) {
            emit statusChanged(
                QStringLiteral("强制终止失败")
            );

            emit logReceived(
                QStringLiteral(
                    "[ERROR] 无法取得当前Abaqus进程PID"
                )
            );

            return;
        }

        const int result =
            QProcess::execute(
                QStringLiteral("taskkill"),
                QStringList()
                    << QStringLiteral("/PID")
                    << QString::number(pid)
                    << QStringLiteral("/T")
                    << QStringLiteral("/F")
            );

        if (abaqusProcess
            && abaqusProcess->state() != QProcess::NotRunning) {
            abaqusProcess->waitForFinished(5000);
        }

        if (abaqusProcess
            && abaqusProcess->state() != QProcess::NotRunning) {
            emit statusChanged(
                QStringLiteral("强制终止失败")
            );

            emit logReceived(
                QStringLiteral(
                    "[ERROR] 强制结束后进程仍然存在"
                )
            );

            return;
        }

        killedTrackedProcessTree = (result == 0);
    }

    if (isCurrentJobLockPresent()) {

        if (!killedTrackedProcessTree) {
            emit statusChanged(
                QStringLiteral("等待Abaqus求解器退出")
            );

            emit logReceived(
                QStringLiteral(
                    "[SYS] Job锁文件仍存在，"
                    "当前没有可安全确认结束的"
                    "Abaqus进程树，继续等待锁文件释放"
                )
            );

            const QString projectPath =
                runningProjectPath.isEmpty()
                    ? m_projectPath
                    : runningProjectPath;

            const QString lockPath =
                QDir(projectPath).filePath(
                    QStringLiteral("abaqus/")
                    + currentJobName
                    + QStringLiteral(".lck")
                );

            waitForJobLockRelease(lockPath);

            return;
        }

        const QString projectPath =
            runningProjectPath.isEmpty()
                ? m_projectPath
                : runningProjectPath;

        const QString lockPath =
            QDir(projectPath).filePath(
                QStringLiteral("abaqus/")
                + currentJobName
                + QStringLiteral(".lck")
            );

        if (!QFile::remove(lockPath)
            && QFile::exists(lockPath)) {
            emit statusChanged(
                QStringLiteral("锁文件清理失败")
            );

            emit logReceived(
                QStringLiteral(
                    "[ERROR] 无法删除终止后残留的锁文件"
                )
            );

            return;
        }
    }

    finishStopState();
}

void SimulationManager::finishStopState()
{
    if (simulationState != SimulationState::Stopping) {
        return;
    }

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        emit logReceived(
            QStringLiteral(
                "[SYS] Abaqus进程仍在运行，"
                "暂不能进入Stopped"
            )
        );
        return;
    }

    if (isCurrentJobLockPresent()) {
        emit logReceived(
            QStringLiteral(
                "[SYS] Job锁文件仍存在，"
                "暂不能进入Stopped"
            )
        );
        return;
    }

    if (simulationTimer) {
        simulationTimer->stop();
    }

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    setSimulationState(SimulationState::Stopped);
    simulationUserStopped = false;
    clearRunningSimulationContext();
    emit simulationStopped();

    emit statusChanged(
        QStringLiteral("仿真已终止")
    );
    emit phaseChanged(
        QStringLiteral("终止")
    );
    emit logReceived(
        QStringLiteral(
            "[SYS] Abaqus已确认退出，仿真终止完成"
        )
    );
}

void SimulationManager::updateAbaqusLog()
{
    readAbaqusLogFile(
        simulationMsgPath,
        QStringLiteral("[MSG]"),
        simulationMsgReadOffset,
        simulationMsgPending
    );

    readAbaqusLogFile(
        simulationStaPath,
        QStringLiteral("[STA]"),
        simulationStaReadOffset,
        simulationStaPending
    );
}

void SimulationManager::readAbaqusLogFile(
    const QString &path,
    const QString &tag,
    qint64 &readOffset,
    QByteArray &pendingData
)
{
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);

    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    if (file.size() < readOffset) {
        readOffset = 0;
        pendingData.clear();
    }

    if (!file.seek(readOffset)) {
        return;
    }

    const QByteArray newData = file.readAll();
    readOffset = file.pos();
    file.close();

    if (newData.isEmpty()) {
        return;
    }

    pendingData.append(newData);

    QList<QByteArray> lines = pendingData.split('\n');

    if (!pendingData.endsWith('\n')) {
        pendingData = lines.takeLast();
    } else {
        pendingData.clear();
    }

    for (const QByteArray &rawLine : lines) {
        const QString line =
            QString::fromLocal8Bit(rawLine).trimmed();

        if (line.isEmpty()) {
            continue;
        }

        emit logReceived(
            tag + QStringLiteral(" ") + line
        );

        if (tag == QStringLiteral("[STA]")) {
            updateProgressFromStaLine(line);
        }
    }
}

void SimulationManager::updateProgressFromStaLine(const QString &line)
{
    if (simulationState != SimulationState::T1Running
        || simulationTotalTime <= 0.0) {
        return;
    }

    const QStringList parts =
        line.simplified().split(
            QLatin1Char(' '),
            Qt::SkipEmptyParts
        );

    if (parts.size() < 7) {
        return;
    }

    bool stepOk = false;
    bool incOk = false;

    parts[0].toInt(&stepOk);
    parts[1].toInt(&incOk);

    if (!stepOk || !incOk) {
        return;
    }

    double currentTime = -1.0;

    for (int i = 2; i < parts.size(); ++i) {
        const QString token = parts[i];

        if (!token.contains(QLatin1Char('.'))
            && !token.contains(QLatin1Char('E'), Qt::CaseInsensitive)) {
            continue;
        }

        bool ok = false;
        const double value = token.toDouble(&ok);

        if (ok && value >= 0.0) {
            currentTime = value;
            break;
        }
    }

    if (currentTime < 0.0) {
        return;
    }

    int percent =
        static_cast<int>(
            currentTime / simulationTotalTime * 100.0
        );

    percent = qBound(0, percent, 99);

    emit progressUpdated(percent);
}

double SimulationManager::loadSimulationTotalTime()
{
    SimulationConfig config;
    if (SimulationConfigManager::load(m_projectPath, config)) {
        return config.timeLength;
    }
    return 0.0;
}

