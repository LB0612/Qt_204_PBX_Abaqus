#include "SimulationManager.h"
#include "SettingsDialog.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QMessageBox>
#include <QTextStream>

SimulationManager::SimulationManager(QObject *parent)
    : QObject(parent)
{}

SimulationManager::~SimulationManager()
{
    // 【核心修复】析构时的安全清理
    // 必须先获取列表副本，因为清理过程中会修改 m_processes
    auto processes = m_processes.values();

    // 使用传统for循环避免Qt容器range-loop detach警告
    for (int i = 0; i < processes.size(); ++i) {
        QProcess *proc = processes[i];
        if (proc) {
            // 1. 必须断开信号，防止触发 handleProcessFinished 导致双重删除
            proc->disconnect();

            // 2. 强杀进程
            if (proc->state() != QProcess::NotRunning) {
                proc->kill();
                proc->waitForFinished(500);
            }

            // 3. 删除进程对象
            delete proc;
        }
    }

    m_processes.clear();
}

SimulationManager& SimulationManager::instance()
{
    static SimulationManager instance;
    return instance;
}

void SimulationManager::startTask(const QString &projectPath, const ProjectConfig &config)
{
    // 检查是否已经在运行
    if (m_processes.contains(projectPath)) {
        QProcess *proc = m_processes[projectPath];
        if (proc && proc->state() == QProcess::Running) {
            emit logReceived(projectPath, "仿真已经在运行中\n");
            return;
        } else {
            // 如果由残留对象但没运行，先清理
            if (proc) delete proc;
            m_processes.remove(projectPath);
        }
    }

    // 准备路径和文件
    QString workDir;
    QString datFileName = "polyflow.dat";
    QDir projectDir(projectPath);

    if (config.processType == 0) {
        workDir = projectDir.filePath("25L/Simulation/mix");
    } else {
        workDir = projectDir.filePath("jiya/Simulation/jiya");
    }
    QString datFullPath = QDir(workDir).filePath(datFileName);

    // 读取maxTime
    double maxTime = readMaxTimeFromDatFile(datFullPath);
    m_maxTimes[projectPath] = maxTime;
    m_projectConfigs[projectPath] = config;

    // 【新增】初始化记忆
    m_executionLogs[projectPath] = "";
    m_currentProgress[projectPath] = 0;
    m_currentStatusText[projectPath] = "准备启动...";

    // 清理旧的sim.pid文件
    QString pidFile = QDir(workDir).filePath("sim.pid");
    QFile::remove(pidFile);

    // 清理旧结果
    QDir outputDir(workDir);
    if (outputDir.cd("Outputs")) {
        outputDir.removeRecursively();
        outputDir.cdUp();
    }
    outputDir.mkdir("Outputs");

    // 创建新进程
    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(workDir);
    proc->setStandardInputFile(datFullPath);

    // 连接信号槽
    connect(proc, &QProcess::readyReadStandardOutput, this, &SimulationManager::handleProcessOutput);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SimulationManager::handleProcessFinished);

    // 保存进程
    m_processes[projectPath] = proc;

    // 启动进程
    QString exePath = SettingsDialog::getPolyflowPath();
    proc->start(exePath, QStringList());

    // 记录PID
    if (proc->waitForStarted(1000)) {
        qint64 pid = proc->processId();
        QFile f(pidFile);
        if (f.open(QIODevice::WriteOnly)) {
            QTextStream out(&f);
            out << pid;
            f.close();
        }
        emit logReceived(projectPath, QString(">>> 进程已启动，PID: %1\n").arg(pid));
    }

    // 初始化进度
    emit progressUpdated(projectPath, 0, "正在启动求解器...");
}

void SimulationManager::stopTask(const QString &projectPath)
{
    if (!m_processes.contains(projectPath)) {
        emit logReceived(projectPath, "当前项目没有正在运行的仿真\n");
        return;
    }

    QProcess *proc = m_processes[projectPath];
    if (!proc) {
        m_processes.remove(projectPath);
        return;
    }

    // 1. 断开信号，防止双重释放
    proc->disconnect();

    // 2. 尝试获取工作目录 (用于后续删文件)
    QString workDir = proc->workingDirectory();

    // 3. 强力终止进程 (Taskkill /F /T)
    qint64 pid = proc->processId();
    if (pid <= 0) {
        // 尝试从文件读取PID补救
        QString pidFile = QDir(workDir).filePath("sim.pid");
        QFile f(pidFile);
        if (f.open(QIODevice::ReadOnly)) {
            pid = f.readAll().toLongLong();
            f.close();
        }
    }

    if (pid > 0) {
        QProcess killer;
        killer.setStandardOutputFile(QProcess::nullDevice());
        killer.setStandardErrorFile(QProcess::nullDevice());
        killer.start("taskkill", QStringList() << "/F" << "/T" << "/PID" << QString::number(pid));
        killer.waitForFinished(2000);
    }

    // 4. 【核心新增】清理生成的垃圾文件 (Outputs 和 Log)
    // (1) 删除 sim.pid
    QString pidFile = QDir(workDir).filePath("sim.pid");
    QFile::remove(pidFile);

    // (2) 删除 Outputs 文件夹 (仿真结果)
    QDir dir(workDir);
    if (dir.cd("Outputs")) {
        dir.removeRecursively(); // 连文件夹带内容一起删，干干净净
    }
    
    // (3) 删除 simulation_realtime.log (实时日志文件)
    QString logFilePath = QDir(projectPath).filePath("simulation_realtime.log");
    QFile::remove(logFilePath);

    // (4) 【关键】清除内存中的“记忆”，防止切回来还能看到旧日志
    if (m_executionLogs.contains(projectPath)) m_executionLogs[projectPath].clear();
    if (m_currentProgress.contains(projectPath)) m_currentProgress[projectPath] = 0;
    if (m_currentStatusText.contains(projectPath)) m_currentStatusText[projectPath] = "已终止，文件已清除";

    // 5. 确保对象销毁
    if (proc->state() != QProcess::NotRunning) {
        proc->kill();
        proc->waitForFinished(1000);
    }
    delete proc;
    m_processes.remove(projectPath);

    // 6. 发送信号通知界面
    emit logReceived(projectPath, ">>> 仿真已终止，已生成的文件和日志已被清除。\n");
    emit progressUpdated(projectPath, 0, "已终止");
    emit taskFinished(projectPath, -1);
}

bool SimulationManager::isRunning(const QString &projectPath)
{
    if (!m_processes.contains(projectPath)) {
        return false;
    }

    QProcess *proc = m_processes[projectPath];
    return proc && proc->state() == QProcess::Running;
}

QString SimulationManager::getProjectLog(const QString &projectPath)
{
    if (m_executionLogs.contains(projectPath)) {
        return m_executionLogs[projectPath];
    }
    return "";
}

int SimulationManager::getProjectProgress(const QString &projectPath)
{
    if (m_currentProgress.contains(projectPath)) {
        return m_currentProgress[projectPath];
    }
    return 0;
}

QString SimulationManager::getProjectStatus(const QString &projectPath)
{
    if (m_currentStatusText.contains(projectPath)) {
        return m_currentStatusText[projectPath];
    }
    return "准备就绪";
}

void SimulationManager::handleProcessOutput()
{
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;

    QString projectPath;
    for (auto it = m_processes.constBegin(); it != m_processes.constEnd(); ++it) {
        if (it.value() == proc) {
            projectPath = it.key();
            break;
        }
    }

    if (projectPath.isEmpty()) return;

    QByteArray data = proc->readAllStandardOutput();
    QString chunk = QString::fromLocal8Bit(data);

    // 1. 实时写入硬盘
    QString logFilePath = QDir(projectPath).filePath("simulation_realtime.log");
    QFile file(logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << chunk;
        file.close();
    }

    // 2. 【新增】存入内存，供界面切换回来时查看
    m_executionLogs[projectPath].append(chunk);
    // 可选：为了防止内存爆炸，可以限制只存最后 10000 字符
    if (m_executionLogs[projectPath].length() > 50000) {
        m_executionLogs[projectPath] = m_executionLogs[projectPath].right(50000);
    }

    // 3. 内存缓冲解析
    m_logBuffers[projectPath].append(chunk);

    QString &buffer = m_logBuffers[projectPath];
    int idx = buffer.indexOf('\n');
    double maxTime = m_maxTimes.value(projectPath, 1.0);

    while (idx != -1) {
        QString line = buffer.left(idx + 1);
        buffer.remove(0, idx + 1);

        emit logReceived(projectPath, line);
        extractProgress(projectPath, line, maxTime);

        idx = buffer.indexOf('\n');
    }
}

void SimulationManager::handleProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status); // 消除未使用变量警告
    QProcess *proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;

    QString projectPath;
    for (auto it = m_processes.constBegin(); it != m_processes.constEnd(); ++it) {
        if (it.value() == proc) {
            projectPath = it.key();
            break;
        }
    }

    if (projectPath.isEmpty()) return;

    // 清理PID
    QString workDir = proc->workingDirectory();
    QString pidFile = QDir(workDir).filePath("sim.pid");
    QFile::remove(pidFile);

    // 从 Map 中移除 (注意：先移除，再 deleteLater)
    m_processes.remove(projectPath);

    // 【新增】更新最终状态
    m_currentProgress[projectPath] = 100;
    m_currentStatusText[projectPath] = exitCode == 0 ? "计算完成" : "计算失败";

    // 发送信号
    emit taskFinished(projectPath, exitCode);
    emit progressUpdated(projectPath, 100, exitCode == 0 ? "计算完成" : "计算失败");

    // 【核心修复】使用 deleteLater() 而不是 delete
    proc->deleteLater();
}

double SimulationManager::readMaxTimeFromDatFile(const QString &datFullPath)
{
    double maxTime = 1.0;
    QFile datFile(datFullPath);
    if (datFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = datFile.readAll();
        datFile.close();

        int nuparStart = content.indexOf("BEGIN NUPAR");
        if (nuparStart != -1) {
            int nuparEnd = content.indexOf("END NUPAR", nuparStart);
            if (nuparEnd == -1) nuparEnd = content.indexOf("ENDOF NUPAR", nuparStart);

            if (nuparEnd != -1) {
                QString nuparBlock = content.mid(nuparStart, nuparEnd - nuparStart);
                // 【优化】使用 static 正则表达式，避免每次调用都重新编译
                static const QRegularExpression d6Re("D\\s+6\\|");
                QRegularExpressionMatch headerMatch = d6Re.match(nuparBlock);

                if (headerMatch.hasMatch()) {
                    int searchStart = headerMatch.capturedEnd();
                    QString valChunk = nuparBlock.mid(searchStart, 200);
                    // 【优化】使用 static 正则表达式，避免每次调用都重新编译
                    static const QRegularExpression numRe(R"([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?)");
                    QRegularExpressionMatchIterator it = numRe.globalMatch(valChunk);

                    int count = 0;
                    while (it.hasNext()) {
                        QRegularExpressionMatch match = it.next();
                        count++;
                        if (count == 2) {
                            maxTime = match.captured().toDouble();
                            break;
                        }
                    }
                }
            }
        }
    }
    if (maxTime <= 1e-6) maxTime = 1.0;
    return maxTime;
}

void SimulationManager::extractProgress(const QString &projectPath, const QString &logLine, double maxTime)
{
    static QRegularExpression timeReg(R"(\bt\s*=\s*([-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?))");
    QRegularExpressionMatch match = timeReg.match(logLine);

    if (match.hasMatch()) {
        double currentT = match.captured(1).toDouble();
        int percent = static_cast<int>((currentT / maxTime) * 100);
        if (percent > 100) percent = 100;

        QString status = QString("正在计算... %1%").arg(percent);
        if (percent >= 100) status = "计算即将完成...";

        // 【新增】记住进度状态
        m_currentProgress[projectPath] = percent;
        m_currentStatusText[projectPath] = status;

        emit progressUpdated(projectPath, percent, status);
    }
}


