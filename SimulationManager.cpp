#include <QProcess>

#include "SimulationManager.h"

#include "ProjectInputHash.h"
#include "ProjectManager.h"
#include "ProjectParameterService.h"
#include "ProjectPaths.h"
#include "SimulationArtifactStateService.h"
#include "SimulationConfigManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>

namespace {

bool isNonEmptyRegularFile(const QString &path)
{
    return SimulationArtifactStateService::isNonEmptyRegularFile(path);
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

bool SimulationManager::isActive() const
{
    return simulationState == SimulationState::T0Running
        || simulationState == SimulationState::T1Running
        || simulationState == SimulationState::T2Running
        || simulationState == SimulationState::Stopping;
}

QString SimulationManager::activeProjectPath() const
{
    return runningProjectPath.isEmpty()
        ? m_projectPath
        : runningProjectPath;
}

QString SimulationManager::activeJobLockPath() const
{
    const QString projectPath = activeProjectPath();
    if (projectPath.isEmpty()) {
        return QString();
    }

    return ProjectInputHash::currentJobLockPath(projectPath);
}

void SimulationManager::setProjectContext(
    const QString &projectPath,
    const QString &abaqusPath)
{
    m_projectPath = projectPath;
    m_abaqusPath = abaqusPath;
}

void SimulationManager::setForceFullRerun(bool forceFullRerun)
{
    m_forceFullRerun = forceFullRerun;
}

void SimulationManager::setSimulationState(SimulationState state)
{
    if (simulationState == state) {
        return;
    }
    simulationState = state;
    emit stateChanged(simulationState);
}

void SimulationManager::startTask(
    const QString &projectPath,
    const QString &abaqusPath)
{
    m_projectPath = projectPath;
    m_abaqusPath = abaqusPath;
    startTaskInternal();
}

SimulationResumeMode SimulationManager::detectResumeMode() const
{
    if (m_forceFullRerun) {
        return SimulationResumeMode::FullRun;
    }

    if (hasCompletePostProcess()) {
        return SimulationResumeMode::AlreadyComplete;
    }

    if (hasValidSolverResult()) {
        return SimulationResumeMode::PostProcessOnly;
    }

    return SimulationResumeMode::FullRun;
}

QString SimulationManager::resumeModeMessage(
    SimulationResumeMode mode) const
{
    switch (mode) {
    case SimulationResumeMode::PostProcessOnly:
        return QStringLiteral(
            "Abaqus 求解已完成，检测到后处理尚未完整完成。"
            "继续运行将仅恢复图片/视频生成，不会重新进行 Abaqus 求解。");
    case SimulationResumeMode::AlreadyComplete:
        return QStringLiteral(
            "当前计算及后处理结果已经完整生成，无需重复运行。");
    case SimulationResumeMode::FullRun:
        return QString();
    }

    return QString();
}

bool SimulationManager::hasValidPreviousResult(QString &message)
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    if (!hasCompletePostProcess()) {
        return false;
    }

    const QString projectDir = m_projectPath;
    const QString odbPath = ProjectInputHash::solverOdbPath(projectDir);
    const QString t2FlagPath =
        ProjectInputHash::t2FinishedFlagPath(projectDir);

    const QFileInfo flagInfo(t2FlagPath);
    const QDateTime completedTime = flagInfo.lastModified();

    message = QStringLiteral(
        "检测到当前工程上一次仿真及后处理已经正常完成，"
        "且当前输入与上一次完全一致。\n\n"
        "完成时间：%1\n"
        "结果文件：%2\n\n"
        "通常不需要重复计算。"
    ).arg(
        completedTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
        odbPath
    );

    return true;
}

bool SimulationManager::readSuccessFlag(const QString &flagPath) const
{
    return SimulationArtifactStateService::readSuccessFlag(flagPath);
}

bool SimulationManager::hasValidSolverResult() const
{
    if (!SimulationArtifactStateService::hasValidSolverResult(m_projectPath)) {
        return false;
    }

    // Opportunistically promote a matching running fingerprint to last-success.
    if (!fingerprintsMatch()) {
        const_cast<SimulationManager *>(this)
            ->recoverSuccessFingerprintIfPossible();
    }

    return true;
}

bool SimulationManager::hasCompletePostProcess() const
{
    if (!SimulationArtifactStateService::hasCompletePostProcess(
            m_projectPath)) {
        return false;
    }

    if (!postFingerprintsMatch()) {
        const_cast<SimulationManager *>(this)
            ->recoverSuccessPostFingerprintIfPossible();
    }

    return true;
}

QString SimulationManager::calculateInputFingerprint() const
{
    return ProjectInputHash::hashSolverInput(m_projectPath);
}

QString SimulationManager::calculatePostFingerprint() const
{
    return ProjectInputHash::hashPostProcessInput(m_projectPath);
}

bool SimulationManager::saveRunFingerprint(
    const QString &filePath,
    const QString &fingerprint) const
{
    if (fingerprint.isEmpty()) {
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    file.write(fingerprint.toLatin1());
    file.write("\n");
    return file.commit();
}

bool SimulationManager::fingerprintMatchesStored(
    const QString &storedPath,
    const QString &currentFingerprint) const
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    return SimulationArtifactStateService::fingerprintFileMatches(
        storedPath,
        currentFingerprint
    );
}

bool SimulationManager::fingerprintsMatch() const
{
    return fingerprintMatchesStored(
        ProjectInputHash::lastSuccessInputFingerprintPath(m_projectPath),
        calculateInputFingerprint()
    );
}

bool SimulationManager::postFingerprintsMatch() const
{
    return fingerprintMatchesStored(
        ProjectInputHash::lastSuccessPostFingerprintPath(m_projectPath),
        calculatePostFingerprint()
    );
}

bool SimulationManager::promoteRunningInputFingerprint(
    const QString &projectPath) const
{
    const QString runningPath =
        ProjectInputHash::runningInputFingerprintPath(projectPath);
    const QString lastPath =
        ProjectInputHash::lastSuccessInputFingerprintPath(projectPath);

    QFile runningFile(runningPath);
    if (!runningFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QByteArray content = runningFile.readAll().trimmed();
    runningFile.close();

    if (content.isEmpty()) {
        return false;
    }

    QSaveFile lastFile(lastPath);
    if (!lastFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    lastFile.write(content);
    lastFile.write("\n");
    if (!lastFile.commit()) {
        return false;
    }

    QFile::remove(runningPath);
    return true;
}

bool SimulationManager::promoteRunningPostFingerprint(
    const QString &projectPath) const
{
    const QString runningPath =
        ProjectInputHash::runningPostFingerprintPath(projectPath);
    const QString lastPath =
        ProjectInputHash::lastSuccessPostFingerprintPath(projectPath);

    QFile runningFile(runningPath);
    if (!runningFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QByteArray content = runningFile.readAll().trimmed();
    runningFile.close();

    if (content.isEmpty()) {
        return false;
    }

    QSaveFile lastFile(lastPath);
    if (!lastFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    lastFile.write(content);
    lastFile.write("\n");
    if (!lastFile.commit()) {
        return false;
    }

    QFile::remove(runningPath);
    return true;
}

bool SimulationManager::commitRunningInputFingerprintFromPrepare(
    QString &errorMessage) const
{
    if (m_projectPath.isEmpty()) {
        errorMessage = QStringLiteral("当前未打开工程。");
        return false;
    }

    const QString preparePath =
        ProjectInputHash::runningInputPrepareFingerprintPath(m_projectPath);
    const QString runningPath =
        ProjectInputHash::runningInputFingerprintPath(m_projectPath);

    QFile prepareFile(preparePath);
    if (!prepareFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMessage = QStringLiteral("无法读取仿真输入准备指纹。");
        return false;
    }

    const QByteArray content = prepareFile.readAll().trimmed();
    prepareFile.close();

    if (content.isEmpty()) {
        errorMessage = QStringLiteral("仿真输入准备指纹为空。");
        return false;
    }

    if (!saveRunFingerprint(
            runningPath,
            QString::fromLatin1(content))) {
        errorMessage = QStringLiteral("无法提交仿真输入指纹。");
        return false;
    }

    QFile::remove(preparePath);
    errorMessage.clear();
    return true;
}

bool SimulationManager::recoverSuccessFingerprintIfPossible()
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    const QString runningPath =
        ProjectInputHash::runningInputFingerprintPath(m_projectPath);

    if (!fingerprintMatchesStored(
            runningPath,
            calculateInputFingerprint())) {
        return false;
    }

    return promoteRunningInputFingerprint(m_projectPath);
}

bool SimulationManager::recoverSuccessPostFingerprintIfPossible()
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    const QString runningPath =
        ProjectInputHash::runningPostFingerprintPath(m_projectPath);

    if (!fingerprintMatchesStored(
            runningPath,
            calculatePostFingerprint())) {
        return false;
    }

    return promoteRunningPostFingerprint(m_projectPath);
}

bool SimulationManager::checkReady(QString &errorMessage) const
{
    const QString projectDir = m_projectPath;

    QString projectNameError;
    if (!ProjectManager::isValidProjectName(QDir(projectDir).dirName(), projectNameError)) {
        errorMessage = QStringLiteral("工程目录名称无效：%1").arg(projectNameError);
        return false;
    }

    const QStringList files = {
        ProjectPaths::t0ScriptPath(projectDir),
        ProjectPaths::t1ScriptPath(projectDir),
        ProjectPaths::t2ScriptPath(projectDir),
        ProjectPaths::userSubroutinePath(projectDir),
    };

    for (const QString &filePath : files) {
        const QFileInfo info(filePath);

        if (!info.exists()) {
            errorMessage =
                QStringLiteral("缺少文件:\n%1").arg(filePath);
            return false;
        }

        if (info.size() <= 0) {
            errorMessage =
                QStringLiteral("Abaqus 生成文件无效：\n%1")
                    .arg(filePath);
            return false;
        }
    }

    const ProjectInputHash::GenerationManifest manifest =
        ProjectInputHash::readGenerationManifest(projectDir);

    if (!manifest.valid) {
        errorMessage = QStringLiteral(
            "Abaqus 文件尚未完整生成，"
            "请重新点击“生成文件”。"
        );
        return false;
    }

    const QString currentConfigHash =
        ProjectInputHash::hashConfigFiles(projectDir);
    const QString currentGeneratedHash =
        ProjectInputHash::hashGeneratedFiles(projectDir);

    if (currentConfigHash.isEmpty() || currentGeneratedHash.isEmpty()) {
        errorMessage = QStringLiteral(
            "无法读取当前参数或 Abaqus 文件内容，"
            "请重新生成文件。"
        );
        return false;
    }

    if (currentConfigHash != manifest.configSha256) {
        errorMessage = QStringLiteral(
            "当前保存参数与已生成的 Abaqus 文件不一致，"
            "请重新生成文件后再开始仿真。"
        );
        return false;
    }

    if (currentGeneratedHash != manifest.generatedSha256) {
        errorMessage = QStringLiteral(
            "当前 Abaqus 文件与生成记录不一致，"
            "请重新生成文件。"
        );
        return false;
    }

    ProjectParameters parameters;
    if (!ProjectParameterService::loadAll(
            projectDir,
            parameters,
            errorMessage)) {
        return false;
    }

    const QString abaqusPath = m_abaqusPath;
    const QFileInfo abaqusInfo(abaqusPath);
    if (abaqusPath.isEmpty() || !abaqusInfo.exists() || !abaqusInfo.isFile()) {
        errorMessage = QStringLiteral("Abaqus路径无效");
        return false;
    }

    return true;
}

void SimulationManager::prepareRunContext()
{
    const QString projectDir = m_projectPath;

    runningProjectPath = projectDir;
    runningAbaqusPath = m_abaqusPath;

    const QString logsDir =
        ProjectPaths::logsDirectoryPath(projectDir);
    QDir().mkpath(logsDir);

    t0LogPath = ProjectPaths::t0LogPath(projectDir);
    t1LogPath = ProjectPaths::t1LogPath(projectDir);
    t2LogPath = ProjectPaths::t2LogPath(projectDir);

    currentJobName = ProjectInputHash::currentJobName(projectDir);

    simulationUserStopped = false;

    emit monitorResetRequested();
    emit progressUpdated(0);
    emit statusChanged(
        QStringLiteral("正在启动 Abaqus...")
    );
    emit logReceived(
        QStringLiteral("[SYS] 开始仿真")
    );
}

bool SimulationManager::clearRunArtifactsForFullRun(QString &errorMessage)
{
    const QString projectDir = m_projectPath;

    auto removeIfExists = [&errorMessage](const QString &path) {
        if (!QFileInfo::exists(path)) {
            return true;
        }

        if (!QFile::remove(path)) {
            errorMessage =
                QStringLiteral("无法清理旧运行文件：\n%1").arg(path);
            return false;
        }

        return true;
    };

    auto clearLog = [&errorMessage](const QString &path) {
        QSaveFile file(path);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            errorMessage =
                QStringLiteral("无法清空日志文件：\n%1").arg(path);
            return false;
        }

        if (!file.commit()) {
            errorMessage =
                QStringLiteral("无法提交空日志文件：\n%1").arg(path);
            return false;
        }

        return true;
    };

    const QStringList removePaths = {
        ProjectPaths::t0FinishedFlagPath(projectDir),
        ProjectInputHash::t1FinishedFlagPath(projectDir),
        ProjectInputHash::t2FinishedFlagPath(projectDir),
        ProjectPaths::currentJobMsgPath(projectDir),
        ProjectPaths::currentJobStaPath(projectDir),
        ProjectPaths::currentJobDatPath(projectDir),
        ProjectInputHash::runningInputFingerprintPath(projectDir),
        ProjectInputHash::runningPostFingerprintPath(projectDir),
        ProjectInputHash::lastSuccessPostFingerprintPath(projectDir),
    };

    for (const QString &path : removePaths) {
        if (!removeIfExists(path)) {
            return false;
        }
    }

    if (!ProjectInputHash::clearPostProcessOutputs(projectDir, errorMessage)) {
        return false;
    }

    if (!clearLog(t0LogPath)
        || !clearLog(t1LogPath)
        || !clearLog(t2LogPath)) {
        return false;
    }

    errorMessage.clear();
    return true;
}

void SimulationManager::updateT2ProgressFromLine(const QString &line)
{
    if (line.isEmpty()) {
        return;
    }

    const QRegularExpression framePattern(
        QStringLiteral("exported frame (\\d+) / (\\d+)")
    );

    const QRegularExpressionMatch match = framePattern.match(line);

    if (match.hasMatch()) {
        const int current = match.captured(1).toInt();
        const int total = match.captured(2).toInt();

        if (total <= 0) {
            return;
        }

        int basePercent = 70;
        int maxPercent = 78;

        if (line.contains(QStringLiteral("[POST] NT11:"))) {
            basePercent = 79;
            maxPercent = 87;
        } else if (line.contains(QStringLiteral("[POST] S:"))) {
            basePercent = 88;
            maxPercent = 96;
        }

        const int percent =
            basePercent
            + static_cast<int>(
                (static_cast<double>(current + 1)
                 / static_cast<double>(total + 1))
                    * static_cast<double>(maxPercent - basePercent)
            );
        emit progressUpdated(
            qBound(basePercent, percent, maxPercent)
        );
    }

    if (line.contains(QStringLiteral("AVI generated"))
        || line.contains(QStringLiteral("Skip existing valid AVI"))) {
        if (line.contains(QStringLiteral("guhuadu"), Qt::CaseInsensitive)) {
            emit progressUpdated(97);
        } else if (line.contains(QStringLiteral("wendu"), Qt::CaseInsensitive)) {
            emit progressUpdated(98);
        } else if (line.contains(QStringLiteral("yingli"), Qt::CaseInsensitive)) {
            emit progressUpdated(99);
        }
    }
}

void SimulationManager::flushT2ProgressPending()
{
    if (t2ProgressPending.isEmpty()) {
        return;
    }

    updateT2ProgressFromLine(
        QString::fromLocal8Bit(t2ProgressPending).trimmed()
    );
    t2ProgressPending.clear();
}

void SimulationManager::appendProcessLog(
    const QString &logPath,
    const QByteArray &data,
    bool isError)
{
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

    if (simulationState == SimulationState::T2Running && !isError) {
        t2ProgressPending.append(data);

        QList<QByteArray> lines = t2ProgressPending.split('\n');

        if (!t2ProgressPending.endsWith('\n')) {
            t2ProgressPending = lines.takeLast();
        } else {
            t2ProgressPending.clear();
        }

        for (const QByteArray &rawLine : lines) {
            updateT2ProgressFromLine(
                QString::fromLocal8Bit(rawLine).trimmed()
            );
        }
    }
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
        emit errorOccurred(
            QStringLiteral("提示"),
            QStringLiteral("检测到当前 Job 锁文件，请先清理后再开始仿真。")
        );
        return;
    }

    QString error;
    if (!checkReady(error)) {
        emit errorOccurred(QStringLiteral("仿真无法启动"), error);
        return;
    }

    const SimulationResumeMode resumeMode = detectResumeMode();
    const bool forceFullRerun = m_forceFullRerun;
    m_forceFullRerun = false;

    if (resumeMode == SimulationResumeMode::AlreadyComplete
        && !forceFullRerun) {
        emit errorOccurred(
            QStringLiteral("提示"),
            resumeModeMessage(SimulationResumeMode::AlreadyComplete)
        );
        return;
    }

    prepareRunContext();

    if (resumeMode == SimulationResumeMode::PostProcessOnly
        && !forceFullRerun) {
        emit logReceived(
            QStringLiteral(
                "[SYS] 检测到有效求解结果，跳过 Abaqus 求解，恢复 t2 后处理"
            )
        );

        updateT2ResetRequestForPostProcessOnly();

        QString postProcessError;
        if (!preparePostProcessRun(postProcessError)) {
            emit errorOccurred(
                QStringLiteral("仿真无法启动"),
                postProcessError
            );
            clearRunningSimulationContext(false);
            return;
        }

        startT2Stage();
        return;
    }

    // Full run
    const QString inputFingerprint = calculateInputFingerprint();
    if (inputFingerprint.isEmpty()) {
        emit errorOccurred(
            QStringLiteral("仿真无法启动"),
            QStringLiteral("无法计算本次仿真输入指纹。")
        );
        clearRunningSimulationContext(false);
        return;
    }

    const QString prepareFingerprintPath =
        ProjectInputHash::runningInputPrepareFingerprintPath(m_projectPath);
    if (!saveRunFingerprint(prepareFingerprintPath, inputFingerprint)) {
        emit errorOccurred(
            QStringLiteral("仿真无法启动"),
            QStringLiteral("无法记录本次仿真输入指纹。")
        );
        clearRunningSimulationContext(false);
        return;
    }

    QString cleanupError;
    if (!clearRunArtifactsForFullRun(cleanupError)) {
        QFile::remove(prepareFingerprintPath);
        emit errorOccurred(QStringLiteral("仿真无法启动"), cleanupError);
        clearRunningSimulationContext(false);
        return;
    }

    if (!commitRunningInputFingerprintFromPrepare(cleanupError)) {
        QFile::remove(prepareFingerprintPath);
        emit errorOccurred(QStringLiteral("仿真无法启动"), cleanupError);
        clearRunningSimulationContext(false);
        return;
    }

    startT0Stage();
}

void SimulationManager::startT0Stage()
{
    const QString projectDir = activeProjectPath();
    const QString abaqusDir =
        ProjectPaths::abaqusDirectoryPath(projectDir);
    const QString t0Path =
        ProjectPaths::t0ScriptPath(projectDir);
    const QString abaqusPath = runningAbaqusPath;

    setSimulationState(SimulationState::T0Running);
    emit progressUpdated(5);

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    QProcess *t0Process = new QProcess(this);
    abaqusProcess = t0Process;
    t0Process->setWorkingDirectory(abaqusDir);

    connect(
        t0Process,
        &QProcess::readyReadStandardOutput,
        this,
        [this, t0Process]() {
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
        [this, t0Process]() {
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
        &SimulationManager::handleT0Finished
    );

    emit logReceived(
        QStringLiteral("[SYS] 启动 t0.py")
    );
    t0Process->start(
        abaqusPath,
        QStringList()
            << QStringLiteral("cae")
            << QStringLiteral("script=") + t0Path
    );

    if (!t0Process->waitForStarted(10000)) {
        if (abaqusProcess == t0Process) {
            abaqusProcess = nullptr;
        }
        t0Process->deleteLater();
        failSolverStage(
            QStringLiteral("无法启动 t0.py"),
            QStringLiteral("[SYS] 无法启动 t0.py"),
            QStringLiteral("无法启动 t0.py。")
        );
        return;
    }

    emit statusChanged(
        QStringLiteral("正在执行 Abaqus")
    );
}

void SimulationManager::handleT0Finished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    drainProcessOutput(t0LogPath);

    QProcess *t0Process = abaqusProcess;
    if (t0Process) {
        if (abaqusProcess == t0Process) {
            abaqusProcess = nullptr;
        }
        t0Process->deleteLater();
    }

    if (simulationUserStopped
        || simulationState == SimulationState::Stopping
        || simulationState == SimulationState::Stopped) {
        return;
    }

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        failSolverStage(
            QStringLiteral("t0.py 执行失败"),
            QStringLiteral("[SYS] t0.py 执行失败"),
            QStringLiteral("t0.py 执行失败。")
        );
        return;
    }

    const QString projectDir = activeProjectPath();
    const QString caePath =
        ProjectPaths::caeModelPath(projectDir);
    if (!isNonEmptyRegularFile(caePath)) {
        failSolverStage(
            QStringLiteral("未生成 guhua.cae"),
            QStringLiteral("[SYS] t0 执行完成，但未生成有效 guhua.cae"),
            QStringLiteral("t0 执行完成，但未生成有效 guhua.cae。")
        );
        return;
    }

    const QString flagPath =
        ProjectPaths::t0FinishedFlagPath(projectDir);
    if (!readSuccessFlag(flagPath)) {
        failSolverStage(
            QStringLiteral("t0执行失败"),
            QStringLiteral("[SYS] t0未正常完成"),
            QStringLiteral("t0 未正常完成（缺少或无效完成标志）。")
        );
        return;
    }

    emit progressUpdated(10);
    emit logReceived(
        QStringLiteral("[SYS] 模型生成完成，启动 t1.py")
    );
    startT1Stage();
}

void SimulationManager::startT1Stage()
{
    const QString projectDir = activeProjectPath();
    const QString abaqusDir =
        ProjectPaths::abaqusDirectoryPath(projectDir);
    const QString t1Path =
        ProjectPaths::t1ScriptPath(projectDir);
    const QString abaqusPath = runningAbaqusPath;

    setSimulationState(SimulationState::T1Running);
    emit statusChanged(
        QStringLiteral("Abaqus 求解中")
    );

    currentJobName = ProjectInputHash::currentJobName(projectDir);

    simulationMsgPath =
        ProjectPaths::currentJobMsgPath(projectDir);
    simulationStaPath =
        ProjectPaths::currentJobStaPath(projectDir);
    const QString simulationDatPath =
        ProjectPaths::currentJobDatPath(projectDir);
    simulationTotalTime = loadSimulationTotalTime();
    simulationMsgReadOffset = 0;
    simulationStaReadOffset = 0;
    simulationMsgPending.clear();
    simulationStaPending.clear();

    QFile::remove(simulationMsgPath);
    QFile::remove(simulationStaPath);
    QFile::remove(simulationDatPath);

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    abaqusProcess = new QProcess(this);
    abaqusProcess->setWorkingDirectory(abaqusDir);

    connect(
        abaqusProcess,
        &QProcess::readyReadStandardOutput,
        this,
        [this]() {
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
        [this]() {
            if (abaqusProcess) {
                appendProcessLog(
                    t1LogPath,
                    abaqusProcess->readAllStandardError(),
                    true
                );
            }
        }
    );

    connect(
        abaqusProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        &SimulationManager::handleT1Finished
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
        if (simulationTimer) {
            simulationTimer->stop();
        }
        QProcess *failedProcess = abaqusProcess;
        abaqusProcess = nullptr;
        if (failedProcess) {
            failedProcess->deleteLater();
        }
        failSolverStage(
            QStringLiteral("无法启动 t1.py"),
            QStringLiteral("[SYS] 无法启动 t1.py"),
            QStringLiteral("无法启动 t1.py。")
        );
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

void SimulationManager::handleT1Finished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    drainProcessOutput(t1LogPath);
    updateAbaqusLog();

    flushAbaqusLogPending(
        QStringLiteral("[MSG]"),
        simulationMsgPending
    );
    flushAbaqusLogPending(
        QStringLiteral("[STA]"),
        simulationStaPending
    );

    if (simulationTimer) {
        simulationTimer->stop();
    }

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    if (simulationUserStopped
        || simulationState == SimulationState::Stopping
        || simulationState == SimulationState::Stopped) {
        return;
    }

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        failSolverStage(
            QStringLiteral("t1.py 执行失败"),
            QStringLiteral("[SYS] t1.py 执行失败"),
            QStringLiteral("t1.py 执行失败。")
        );
        return;
    }

    const QString projectDir = activeProjectPath();
    const QString odbPath = ProjectInputHash::solverOdbPath(projectDir);
    const QString t1FlagPath =
        ProjectInputHash::t1FinishedFlagPath(projectDir);

    if (!isNonEmptyRegularFile(odbPath)) {
        failSolverStage(
            QStringLiteral("Abaqus 未生成有效计算结果"),
            QStringLiteral("[SYS] Abaqus 未生成有效 ODB"),
            QStringLiteral(
                "Abaqus 未生成有效计算结果，"
                "ODB 不存在或为空。"
            )
        );
        return;
    }

    if (!readSuccessFlag(t1FlagPath)) {
        failSolverStage(
            QStringLiteral("t1未正常完成"),
            QStringLiteral("[SYS] t1未正常完成"),
            QStringLiteral("t1 未正常完成（缺少或无效完成标志）。")
        );
        return;
    }

    const bool promoted =
        promoteRunningInputFingerprint(projectDir);
    if (!promoted) {
        emit logReceived(
            QStringLiteral(
                "[SYS] 求解成功指纹写入失败，"
                "将在下次打开时恢复"
            )
        );
    } else {
        emit logReceived(
            QStringLiteral("[SYS] 求解成功指纹已保存")
        );
    }

    emit logReceived(
        QStringLiteral("[SYS] Abaqus 求解完成，启动后处理")
    );

    m_t2ResetRequested = true;

    QString postProcessError;
    if (!preparePostProcessRun(postProcessError)) {
        failPostProcessStage(
            QStringLiteral("无法准备后处理"),
            QStringLiteral("[SYS] 无法准备后处理输出"),
            postProcessError
        );
        return;
    }

    startT2Stage();
}

QProcessEnvironment SimulationManager::buildT2ProcessEnvironment() const
{
    QProcessEnvironment env =
        QProcessEnvironment::systemEnvironment();

    env.insert(
        QStringLiteral("PBX_POST_SHA256"),
        calculatePostFingerprint()
    );
    env.insert(
        QStringLiteral("PBX_T2_RESET"),
        m_t2ResetRequested ? QStringLiteral("1") : QStringLiteral("0")
    );

    return env;
}

void SimulationManager::updateT2ResetRequestForPostProcessOnly()
{
    const QString currentPost = calculatePostFingerprint();
    const QString runningPath =
        ProjectInputHash::runningPostFingerprintPath(m_projectPath);

    m_t2ResetRequested = true;

    if (fingerprintMatchesStored(runningPath, currentPost)) {
        m_t2ResetRequested = false;
    }
}

bool SimulationManager::preparePostProcessRun(QString &errorMessage)
{
    const QString projectDir = activeProjectPath();
    if (projectDir.isEmpty()) {
        errorMessage = QStringLiteral("当前未打开工程。");
        return false;
    }

    const QString resultsDir =
        ProjectPaths::resultsDirectoryPath(projectDir);

    if (!QDir(resultsDir).exists()
        && !QDir().mkpath(resultsDir)) {
        errorMessage = QStringLiteral(
            "无法创建后处理结果目录：\n%1"
        ).arg(resultsDir);
        return false;
    }

    if (m_t2ResetRequested) {
        QFile::remove(
            ProjectInputHash::lastSuccessPostFingerprintPath(projectDir)
        );

        if (!ProjectInputHash::clearPostProcessOutputs(
                projectDir,
                errorMessage)) {
            return false;
        }
    }

    const QString postFingerprint = calculatePostFingerprint();
    if (postFingerprint.isEmpty()) {
        errorMessage = QStringLiteral("无法计算本次后处理输入指纹。");
        return false;
    }

    if (!saveRunFingerprint(
            ProjectInputHash::runningPostFingerprintPath(projectDir),
            postFingerprint)) {
        errorMessage = QStringLiteral("无法记录本次后处理输入指纹。");
        return false;
    }

    errorMessage.clear();
    return true;
}

void SimulationManager::startT2Stage()
{
    const QString projectDir = activeProjectPath();
    const QString abaqusDir =
        ProjectPaths::abaqusDirectoryPath(projectDir);
    const QString t2Path =
        ProjectPaths::t2ScriptPath(projectDir);
    const QString abaqusPath = runningAbaqusPath;

    setSimulationState(SimulationState::T2Running);
    emit statusChanged(
        QStringLiteral("后处理中")
    );
    emit progressUpdated(70);

    emit logReceived(
        m_t2ResetRequested
            ? QStringLiteral("[SYS] 启动 t2.py（重置后处理输出）")
            : QStringLiteral("[SYS] 启动 t2.py（续传后处理）")
    );

    t2ProgressPending.clear();

    QFile t2LogFile(t2LogPath);
    if (t2LogFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        t2LogFile.close();
    }

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    abaqusProcess = new QProcess(this);
    abaqusProcess->setWorkingDirectory(abaqusDir);
    abaqusProcess->setProcessEnvironment(buildT2ProcessEnvironment());

    connect(
        abaqusProcess,
        &QProcess::readyReadStandardOutput,
        this,
        [this]() {
            if (abaqusProcess) {
                appendProcessLog(
                    t2LogPath,
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
        [this]() {
            if (abaqusProcess) {
                appendProcessLog(
                    t2LogPath,
                    abaqusProcess->readAllStandardError(),
                    true
                );
            }
        }
    );

    connect(
        abaqusProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        &SimulationManager::handleT2Finished
    );

    emit logReceived(
        QStringLiteral("[SYS] 启动 t2.py")
    );
    abaqusProcess->start(
        abaqusPath,
        QStringList()
            << QStringLiteral("cae")
            << QStringLiteral("script=") + t2Path
    );

    if (!abaqusProcess->waitForStarted(10000)) {
        QProcess *failedProcess = abaqusProcess;
        abaqusProcess = nullptr;
        if (failedProcess) {
            failedProcess->deleteLater();
        }
        failPostProcessStage(
            QStringLiteral("无法启动 t2.py"),
            QStringLiteral("[SYS] 无法启动 t2.py"),
            QStringLiteral(
                "Abaqus 求解已经成功，但无法启动后处理进程。"
            )
        );
    }
}

void SimulationManager::handleT2Finished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    drainProcessOutput(t2LogPath);
    flushT2ProgressPending();

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    if (simulationUserStopped
        || simulationState == SimulationState::Stopping
        || simulationState == SimulationState::Stopped) {
        return;
    }

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        failPostProcessStage(
            QStringLiteral("t2.py 执行失败"),
            QStringLiteral("[ERROR] Post-processing failed."),
            QStringLiteral(
                "Abaqus 求解已经成功，ODB 结果已安全保存，"
                "但后处理未完整完成。再次开始仿真时将仅继续后处理，"
                "不会重新进行 Abaqus 求解。"
            )
        );
        return;
    }

    const QString projectDir = activeProjectPath();
    const QString t2FlagPath =
        ProjectInputHash::t2FinishedFlagPath(projectDir);

    if (!readSuccessFlag(t2FlagPath)) {
        failPostProcessStage(
            QStringLiteral("后处理未正常完成"),
            QStringLiteral("[ERROR] Post-processing failed."),
            QStringLiteral(
                "Abaqus 求解已经成功，ODB 结果已安全保存，"
                "但后处理未完整完成。再次开始仿真时将仅继续后处理，"
                "不会重新进行 Abaqus 求解。"
            )
        );
        return;
    }

    const ProjectInputHash::PostProcessManifest manifest =
        ProjectInputHash::readPostProcessManifest(projectDir);

    if (!manifest.valid) {
        failPostProcessStage(
            QStringLiteral("后处理清单无效"),
            QStringLiteral("[ERROR] Post-processing failed."),
            QStringLiteral(
                "Abaqus 求解已经成功，ODB 结果已安全保存，"
                "但后处理清单无效。再次开始仿真时将仅继续后处理，"
                "不会重新进行 Abaqus 求解。"
            )
        );
        return;
    }

    const QString currentPost = calculatePostFingerprint();
    if (currentPost.isEmpty()
        || manifest.postSha256 != currentPost) {
        failPostProcessStage(
            QStringLiteral("后处理指纹不匹配"),
            QStringLiteral("[ERROR] Post-processing failed."),
            QStringLiteral(
                "Abaqus 求解已经成功，ODB 结果已安全保存，"
                "但后处理指纹不匹配。再次开始仿真时将仅继续后处理，"
                "不会重新进行 Abaqus 求解。"
            )
        );
        return;
    }

    QString outputError;
    if (!ProjectInputHash::validatePostProcessOutputs(
            projectDir,
            outputError)) {
        failPostProcessStage(
            QStringLiteral("后处理输出校验失败"),
            QStringLiteral(
                "[ERROR] Post-processing "
                "output validation failed."
            ),
            QStringLiteral(
                "Abaqus 求解已经成功，"
                "但后处理输出文件未通过最终完整性检查。\n\n"
                "原因：%1\n\n"
                "再次开始仿真时将仅继续后处理，"
                "不会重新进行 Abaqus 求解。"
            ).arg(outputError)
        );
        return;
    }

    const bool promoted =
        promoteRunningPostFingerprint(projectDir);
    if (!promoted) {
        emit logReceived(
            QStringLiteral(
                "[SYS] 后处理成功指纹写入失败，"
                "将在下次打开时恢复"
            )
        );
    }

    finishSimulationSuccess(promoted);
}

void SimulationManager::finishSimulationSuccess(bool postPromoted)
{
    setSimulationState(SimulationState::Finished);
    clearRunningSimulationContext(false);
    clearRunningPostContext(postPromoted);

    emit statusChanged(
        QStringLiteral("固化仿真完成")
    );
    emit progressUpdated(100);
    emit logReceived(
        QStringLiteral("[SYS] 固化仿真完成")
    );
    emit simulationFinished();
}

void SimulationManager::failSolverStage(
    const QString &statusText,
    const QString &logText,
    const QString &errorText)
{
    setSimulationState(SimulationState::Failed);
    clearRunningSimulationContext(true);
    clearRunningPostContext(true);

    emit statusChanged(statusText);
    emit logReceived(logText);
    emit errorOccurred(QStringLiteral("错误"), errorText);
}

void SimulationManager::failPostProcessStage(
    const QString &statusText,
    const QString &logText,
    const QString &errorText)
{
    setSimulationState(SimulationState::PostProcessFailed);

    emit logReceived(
        QStringLiteral("[SYS] Abaqus solver result preserved.")
    );
    emit statusChanged(statusText);
    emit logReceived(logText);
    emit logReceived(
        QStringLiteral("[SYS] Next run will resume t2 only.")
    );
    emit errorOccurred(
        QStringLiteral("后处理未完成"),
        errorText
    );

    clearRunningSimulationContext(false);
}

void SimulationManager::clearRunningSimulationContext(
    bool removeRunningFingerprint)
{
    const QString projectPath = activeProjectPath();
    if (removeRunningFingerprint && !projectPath.isEmpty()) {
        QFile::remove(
            ProjectInputHash::runningInputFingerprintPath(projectPath)
        );
        QFile::remove(
            ProjectInputHash::runningInputPrepareFingerprintPath(projectPath)
        );
    }

    currentJobName.clear();
    runningProjectPath.clear();
    runningAbaqusPath.clear();

    simulationMsgPath.clear();
    simulationStaPath.clear();

    simulationMsgReadOffset = 0;
    simulationStaReadOffset = 0;
    simulationMsgPending.clear();
    simulationStaPending.clear();
    t2ProgressPending.clear();
    simulationTotalTime = 0.0;
}

void SimulationManager::clearRunningPostContext(
    bool removeRunningPostFingerprint)
{
    const QString projectPath = activeProjectPath();
    if (removeRunningPostFingerprint && !projectPath.isEmpty()) {
        QFile::remove(
            ProjectInputHash::runningPostFingerprintPath(projectPath)
        );
    }
}

void SimulationManager::stopTask()
{
    const SimulationState sourceState = simulationState;

    if (sourceState != SimulationState::T0Running
        && sourceState != SimulationState::T1Running
        && sourceState != SimulationState::T2Running) {
        return;
    }

    m_stopSourceState = sourceState;
    simulationUserStopped = true;
    setSimulationState(SimulationState::Stopping);

    if (sourceState == SimulationState::T0Running) {
        emit statusChanged(
            QStringLiteral("正在停止模型建立")
        );
        emit logReceived(
            QStringLiteral("[SYS] 用户请求停止模型建立")
        );
        emit logReceived(
            QStringLiteral(
                "[SYS] 当前 Job 尚未启动，"
                "正在请求模型建立进程退出"
            )
        );

        closeAbaqusProcesses();
        return;
    }

    if (sourceState == SimulationState::T2Running) {
        emit statusChanged(
            QStringLiteral("正在停止后处理")
        );
        emit logReceived(
            QStringLiteral("[SYS] 用户请求停止后处理")
        );
        emit logReceived(
            QStringLiteral(
                "[SYS] 当前无运行中的 Job，"
                "正在请求后处理进程退出"
            )
        );

        closeAbaqusProcesses();
        return;
    }

    emit statusChanged(
        QStringLiteral("正在请求终止")
    );
    emit logReceived(
        QStringLiteral("[SYS] 用户请求终止仿真")
    );

    sendAbaqusTerminateCommand();
}

bool SimulationManager::hasLockFiles() const
{
    if (m_projectPath.isEmpty()) {
        return false;
    }

    return QFile::exists(currentJobLockPath());
}

QString SimulationManager::currentJobLockPath() const
{
    if (m_projectPath.isEmpty()) {
        return QString();
    }

    return ProjectInputHash::currentJobLockPath(m_projectPath);
}

bool SimulationManager::clearCurrentJobLock(QString &errorMessage) const
{
    const QString lockPath = currentJobLockPath();
    if (lockPath.isEmpty()) {
        errorMessage = QStringLiteral("当前未打开工程。");
        return false;
    }

    if (!QFile::exists(lockPath)) {
        errorMessage = QStringLiteral("当前 Job 锁文件不存在。");
        return false;
    }

    if (!QFile::remove(lockPath)) {
        errorMessage = QStringLiteral("无法删除当前 Job 锁文件。");
        return false;
    }

    return true;
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

void SimulationManager::handleTerminateRequestFailure(const QString &reason)
{
    emit logReceived(
        QStringLiteral("[ERROR] ") + reason
    );

    const QString lockPath = activeJobLockPath();

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

void SimulationManager::sendAbaqusTerminateCommand()
{
    const QString projectPath = activeProjectPath();
    const QString abaqusPath = runningAbaqusPath.isEmpty()
        ? m_abaqusPath
        : runningAbaqusPath;

    if (abaqusPath.isEmpty() || !QFile::exists(abaqusPath)) {
        handleTerminateRequestFailure(
            QStringLiteral("Abaqus 路径无效，无法发送 terminate")
        );
        return;
    }

    const QString abaqusDir =
        ProjectPaths::abaqusDirectoryPath(projectPath);
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
                QString::fromLocal8Bit(
                    terminateProcess->readAllStandardOutput()
                );
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
                QString::fromLocal8Bit(
                    terminateProcess->readAllStandardError()
                );
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
        [this, terminateProcess, projectPath](
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
                ProjectInputHash::currentJobLockPath(projectPath);

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
                "[SYS] 无法取得进程树 PID，"
                "尝试直接终止 QProcess"
            )
        );

        abaqusProcess->kill();
        abaqusProcess->waitForFinished(5000);

        if (abaqusProcess->state() != QProcess::NotRunning) {
            restoreRunningStateAfterStopFailure(
                QStringLiteral("无法结束当前 Abaqus 进程。")
            );
            return;
        }

        finishStopState(true);
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
        abaqusProcess->kill();
        abaqusProcess->waitForFinished(5000);
    }

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        restoreRunningStateAfterStopFailure(
            QStringLiteral(
                "taskkill 和 QProcess::kill "
                "均未能结束当前 Abaqus 进程。"
            )
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

    finishStopState(true);
}

void SimulationManager::restoreRunningStateAfterStopFailure(
    const QString &reason)
{
    simulationUserStopped = false;

    emit logReceived(
        QStringLiteral("[ERROR] ") + reason
    );

    if (m_stopSourceState == SimulationState::T1Running
        && simulationTimer
        && abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        simulationTimer->start(1000);
    }

    const SimulationState restoreState = m_stopSourceState;
    m_stopSourceState = SimulationState::Idle;
    setSimulationState(restoreState);

    emit statusChanged(
        QStringLiteral("终止失败，原 Abaqus 任务仍在运行")
    );
}

void SimulationManager::drainProcessOutput(const QString &logPath)
{
    if (!abaqusProcess) {
        return;
    }

    appendProcessLog(
        logPath,
        abaqusProcess->readAllStandardOutput(),
        false
    );
    appendProcessLog(
        logPath,
        abaqusProcess->readAllStandardError(),
        true
    );
}

bool SimulationManager::isCurrentJobLockPresent() const
{
    if (currentJobName.isEmpty()) {
        return false;
    }

    const QString lockPath = activeJobLockPath();
    return !lockPath.isEmpty() && QFile::exists(lockPath);
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
            restoreRunningStateAfterStopFailure(
                QStringLiteral(
                    "无法取得当前 Abaqus 进程 PID，"
                    "未能执行强制结束。"
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

            abaqusProcess->kill();
            abaqusProcess->waitForFinished(5000);

            if (abaqusProcess
                && abaqusProcess->state()
                    != QProcess::NotRunning) {

                restoreRunningStateAfterStopFailure(
                    QStringLiteral(
                        "强制结束后 Abaqus 进程仍然存在。"
                    )
                );

                return;
            }
        }

        killedTrackedProcessTree = (result == 0);
    }

    if (isCurrentJobLockPresent()) {

        if (!killedTrackedProcessTree) {
            emit logReceived(
                QStringLiteral(
                    "[SYS] 当前跟踪的Abaqus进程已经退出，"
                    "但无法确认taskkill成功且Job锁仍存在。"
                    "将保留残留锁并完成终止，"
                    "下次启动时重新检查。"
                )
            );

            finishStopState(true);
            return;
        }

        const QString lockPath = activeJobLockPath();

        if (!QFile::remove(lockPath)
            && QFile::exists(lockPath)) {

            emit logReceived(
                QStringLiteral(
                    "[SYS] Abaqus进程树已经结束，"
                    "但残留Job锁无法删除，"
                    "将在下次启动时重新检查。"
                )
            );

            finishStopState(true);
            return;
        }
    }

    finishStopState();
}

void SimulationManager::finishStopState(bool allowStaleLock)
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
        if (!allowStaleLock) {
            emit logReceived(
                QStringLiteral(
                    "[SYS] Job锁文件仍存在，"
                    "暂不能进入Stopped"
                )
            );
            return;
        }

        emit logReceived(
            QStringLiteral(
                "[SYS] Abaqus进程已经确认退出，"
                "但Job锁文件仍存在。"
                "将保留残留锁并进入Stopped，"
                "下次启动时再执行残留锁检查。"
            )
        );
    }

    if (simulationTimer) {
        simulationTimer->stop();
    }

    if (m_stopSourceState == SimulationState::T1Running) {
        updateAbaqusLog();
        flushAbaqusLogPending(
            QStringLiteral("[MSG]"),
            simulationMsgPending
        );
        flushAbaqusLogPending(
            QStringLiteral("[STA]"),
            simulationStaPending
        );
    }

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    const QString projectPath = activeProjectPath();
    bool preserveResults = false;

    if (!projectPath.isEmpty()
        && readSuccessFlag(
            ProjectInputHash::t1FinishedFlagPath(projectPath))
        && isNonEmptyRegularFile(
            ProjectInputHash::solverOdbPath(projectPath))) {
        const QString currentSolverSha = calculateInputFingerprint();
        if (!currentSolverSha.isEmpty()) {
            const bool lastMatches = fingerprintMatchesStored(
                ProjectInputHash::lastSuccessInputFingerprintPath(projectPath),
                currentSolverSha
            );
            const bool runningMatches = fingerprintMatchesStored(
                ProjectInputHash::runningInputFingerprintPath(projectPath),
                currentSolverSha
            );

            if (lastMatches || runningMatches) {
                preserveResults = true;

                if (runningMatches
                    && !promoteRunningInputFingerprint(projectPath)) {
                    emit logReceived(
                        QStringLiteral(
                            "[SYS] 求解成功指纹写入失败，"
                            "保留 running fingerprint 供下次恢复"
                        )
                    );
                }
            }
        }
    }

    m_stopSourceState = SimulationState::Idle;
    setSimulationState(SimulationState::Stopped);
    simulationUserStopped = false;
    clearRunningSimulationContext(!preserveResults);
    clearRunningPostContext(!preserveResults);

    emit statusChanged(
        QStringLiteral("仿真已终止")
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

void SimulationManager::flushAbaqusLogPending(
    const QString &tag,
    QByteArray &pendingData)
{
    if (pendingData.isEmpty()) {
        return;
    }

    const QString line =
        QString::fromLocal8Bit(pendingData).trimmed();

    pendingData.clear();

    if (line.isEmpty()) {
        return;
    }

    emit logReceived(
        tag + QStringLiteral(" ") + line
    );

    if (tag == QStringLiteral("[STA]")) {
        updateProgressFromStaLine(line);
    }
}

void SimulationManager::readAbaqusLogFile(
    const QString &path,
    const QString &tag,
    qint64 &readOffset,
    QByteArray &pendingData)
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

    QStringList displayLines;

    for (const QByteArray &rawLine : lines) {
        const QString line =
            QString::fromLocal8Bit(rawLine).trimmed();

        if (line.isEmpty()) {
            continue;
        }

        displayLines.append(
            tag + QStringLiteral(" ") + line
        );

        if (tag == QStringLiteral("[STA]")) {
            updateProgressFromStaLine(line);
        }
    }

    if (!displayLines.isEmpty()) {
        emit logReceived(
            displayLines.join(QLatin1Char('\n'))
        );
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
        10 + static_cast<int>(
            currentTime / simulationTotalTime * 60.0
        );

    percent = qBound(10, percent, 70);

    emit progressUpdated(percent);
}

double SimulationManager::loadSimulationTotalTime()
{
    const QString projectPath = activeProjectPath();
    if (projectPath.isEmpty()) {
        return 0.0;
    }

    SimulationConfig config;
    if (SimulationConfigManager::load(projectPath, config)) {
        return config.timeLength;
    }
    return 0.0;
}
