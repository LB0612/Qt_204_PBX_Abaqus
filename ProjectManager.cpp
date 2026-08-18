#include "ProjectManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>

namespace {
const QString FILE_PROJECT = QStringLiteral("project.json");
const QString PROJECT_TYPE_PBX = QStringLiteral("PBX_CASTING_CURING");
}

bool ProjectManager::createProject(
    const QString &basePath,
    const QString &projectName,
    ProjectConfig &config,
    QString &errorMessage)
{
    const QString name = projectName.trimmed();

    if (name.isEmpty()) {
        errorMessage = QStringLiteral("工程名称不能为空。");
        return false;
    }

    static const QRegularExpression invalid(R"([\\/:*?\"<>|])");
    if (name.contains(invalid)) {
        errorMessage = QStringLiteral("工程名称包含非法字符。");
        return false;
    }

    QDir baseDir(basePath);
    if (!baseDir.exists()) {
        errorMessage = QStringLiteral("工程保存位置不存在。");
        return false;
    }

    const QString projectPath = baseDir.filePath(name);
    if (QDir(projectPath).exists()) {
        errorMessage = QStringLiteral("同名工程已经存在。");
        return false;
    }

    QDir dir;
    if (!dir.mkpath(projectPath)) {
        errorMessage = QStringLiteral("无法创建工程目录。");
        return false;
    }

    QDir projectDir(projectPath);
    if (!projectDir.mkdir(QStringLiteral("config"))
        || !projectDir.mkdir(QStringLiteral("abaqus"))
        || !projectDir.mkdir(QStringLiteral("results"))
        || !projectDir.mkdir(QStringLiteral("logs"))) {
        QDir(projectPath).removeRecursively();
        errorMessage = QStringLiteral("无法创建工程子目录。");
        return false;
    }

    config.projectName = name;
    config.projectPath = projectPath;
    config.projectType = PROJECT_TYPE_PBX;
    config.createdDate = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    config.softwareVersion = QStringLiteral("1.0");
    config.schemaVersion = 1;

    if (!saveProject(projectPath, config)) {
        QDir(projectPath).removeRecursively();
        errorMessage = QStringLiteral("工程配置文件创建失败。");
        return false;
    }

    return true;
}

bool ProjectManager::saveProject(
    const QString &folderPath,
    const ProjectConfig &config)
{
    QJsonObject jsonObj;
    jsonObj[QStringLiteral("projectName")] = config.projectName;
    jsonObj[QStringLiteral("projectType")] = config.projectType;
    jsonObj[QStringLiteral("createdDate")] = config.createdDate;
    jsonObj[QStringLiteral("softwareVersion")] = config.softwareVersion;
    jsonObj[QStringLiteral("schemaVersion")] = config.schemaVersion;

    QSaveFile file(QDir(folderPath).filePath(FILE_PROJECT));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool ProjectManager::loadProject(
    const QString &folderPath,
    ProjectConfig &config)
{
    QDir dir(folderPath);
    if (!dir.exists()) {
        return false;
    }

    QFile file(dir.filePath(FILE_PROJECT));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QJsonObject jsonObj = QJsonDocument::fromJson(file.readAll()).object();
    if (jsonObj.value(QStringLiteral("projectType")).toString() != PROJECT_TYPE_PBX) {
        return false;
    }

    config.projectName = dir.dirName();
    config.projectPath = folderPath;
    config.projectType = jsonObj.value(QStringLiteral("projectType")).toString(PROJECT_TYPE_PBX);
    config.createdDate = jsonObj.value(QStringLiteral("createdDate")).toString();
    config.softwareVersion = jsonObj.value(QStringLiteral("softwareVersion")).toString(QStringLiteral("1.0"));
    config.schemaVersion = jsonObj.value(QStringLiteral("schemaVersion")).toInt(1);
    return true;
}

bool ProjectManager::isValidProject(const QString &folderPath)
{
    ProjectConfig config;
    return loadProject(folderPath, config);
}
