#ifndef PROJECTPATHS_H
#define PROJECTPATHS_H

#include <QDir>
#include <QString>
#include <QStringList>

namespace ProjectPaths {

inline QString projectJsonFileName()
{
    return QStringLiteral("project.json");
}

inline QString configDirName()
{
    return QStringLiteral("config");
}

inline QString abaqusDirName()
{
    return QStringLiteral("abaqus");
}

inline QString resultsDirName()
{
    return QStringLiteral("results");
}

inline QString logsDirName()
{
    return QStringLiteral("logs");
}

inline QString structureConfigRelativePath()
{
    return QStringLiteral("config/structure.json");
}

inline QString explosiveConfigRelativePath()
{
    return QStringLiteral("config/explosive.json");
}

inline QString moldConfigRelativePath()
{
    return QStringLiteral("config/mold.json");
}

inline QString boundaryConfigRelativePath()
{
    return QStringLiteral("config/boundary.json");
}

inline QString simulationConfigRelativePath()
{
    return QStringLiteral("config/simulation.json");
}

inline QStringList parameterConfigRelativePaths()
{
    return {
        structureConfigRelativePath(),
        explosiveConfigRelativePath(),
        moldConfigRelativePath(),
        boundaryConfigRelativePath(),
        simulationConfigRelativePath(),
    };
}

inline QString t0ScriptRelativePath()
{
    return QStringLiteral("abaqus/t0.py");
}

inline QString t1ScriptRelativePath()
{
    return QStringLiteral("abaqus/t1.py");
}

inline QString t2ScriptRelativePath()
{
    return QStringLiteral("abaqus/t2.py");
}

inline QString userSubroutineRelativePath()
{
    return QStringLiteral("abaqus/335K.for");
}

inline QStringList generatedAbaqusRelativePaths()
{
    return {
        t0ScriptRelativePath(),
        t1ScriptRelativePath(),
        t2ScriptRelativePath(),
        userSubroutineRelativePath(),
    };
}

inline QString projectJsonPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(projectJsonFileName());
}

inline QString configDirectoryPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(configDirName());
}

inline QString abaqusDirectoryPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(abaqusDirName());
}

inline QString resultsDirectoryPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(resultsDirName());
}

inline QString logsDirectoryPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(logsDirName());
}

inline QString structureConfigPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(structureConfigRelativePath());
}

inline QString explosiveConfigPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(explosiveConfigRelativePath());
}

inline QString moldConfigPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(moldConfigRelativePath());
}

inline QString boundaryConfigPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(boundaryConfigRelativePath());
}

inline QString simulationConfigPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(simulationConfigRelativePath());
}

inline QString t0ScriptPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(t0ScriptRelativePath());
}

inline QString t1ScriptPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(t1ScriptRelativePath());
}

inline QString t2ScriptPath(const QString &projectPath)
{
    return QDir(projectPath).filePath(t2ScriptRelativePath());
}

inline QString userSubroutinePath(const QString &projectPath)
{
    return QDir(projectPath).filePath(userSubroutineRelativePath());
}

inline QString caeModelPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("guhua.cae"));
}

inline QString generationCompleteFlagPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("generation_complete.flag"));
}

inline QString postProcessManifestPath(const QString &projectPath)
{
    return QDir(resultsDirectoryPath(projectPath))
        .filePath(QStringLiteral("postprocess_manifest.json"));
}

inline QString t0FinishedFlagPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("t0_finished.flag"));
}

inline QString t1FinishedFlagPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("t1_finished.flag"));
}

inline QString t2FinishedFlagPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("t2_finished.flag"));
}

inline QString runningInputPrepareFingerprintPath(
    const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("running_input.prepare.sha256"));
}

inline QString runningInputFingerprintPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("running_input.sha256"));
}

inline QString lastSuccessInputFingerprintPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(QStringLiteral("last_success_input.sha256"));
}

inline QString runningPostFingerprintPath(const QString &projectPath)
{
    return QDir(resultsDirectoryPath(projectPath))
        .filePath(QStringLiteral("running_post_input.sha256"));
}

inline QString lastSuccessPostFingerprintPath(const QString &projectPath)
{
    return QDir(resultsDirectoryPath(projectPath))
        .filePath(QStringLiteral("last_success_post.sha256"));
}

inline QString currentJobName(const QString &projectPath)
{
    return QDir(projectPath).dirName() + QStringLiteral("_Job");
}

inline QString solverOdbPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(currentJobName(projectPath) + QStringLiteral(".odb"));
}

inline QString currentJobLockPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(currentJobName(projectPath) + QStringLiteral(".lck"));
}

inline QString currentJobMsgPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(currentJobName(projectPath) + QStringLiteral(".msg"));
}

inline QString currentJobStaPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(currentJobName(projectPath) + QStringLiteral(".sta"));
}

inline QString currentJobDatPath(const QString &projectPath)
{
    return QDir(abaqusDirectoryPath(projectPath))
        .filePath(currentJobName(projectPath) + QStringLiteral(".dat"));
}

} // namespace ProjectPaths

#endif
