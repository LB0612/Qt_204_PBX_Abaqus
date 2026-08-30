#include "ProjectManager.h"

#include "ProjectPaths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>

bool ProjectManager::isValidProjectName(
    const QString &projectName,
    QString &errorMessage)
{
    const QString name = projectName.trimmed();

    if (name.isEmpty()) {
        errorMessage = QStringLiteral("工程名称不能为空。");
        return false;
    }

    static const QRegularExpression valid(
        QStringLiteral("^[A-Za-z0-9_-]+$")
    );
    if (!valid.match(name).hasMatch()) {
        errorMessage = QStringLiteral(
            "工程名称只能包含英文字母、数字、"
            "下划线（_）和连字符（-）。"
        );
        return false;
    }

    return true;
}

bool ProjectManager::createProject(
    const QString &basePath,
    const QString &projectName,
    ProjectConfig &config,
    QString &errorMessage)
{
    const QString name = projectName.trimmed();

    if (!isValidProjectName(name, errorMessage)) {
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
    if (!projectDir.mkdir(ProjectPaths::configDirName())
        || !projectDir.mkdir(ProjectPaths::abaqusDirName())
        || !projectDir.mkdir(ProjectPaths::resultsDirName())
        || !projectDir.mkdir(ProjectPaths::logsDirName())) {
        QDir(projectPath).removeRecursively();
        errorMessage = QStringLiteral("无法创建工程子目录。");
        return false;
    }

    config.projectName = name;
    config.projectPath = projectPath;
    config.projectType = AppInfo::ProjectTypeId;
    config.createdDate = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
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
    jsonObj[QStringLiteral("schemaVersion")] = config.schemaVersion;

    QSaveFile file(ProjectPaths::projectJsonPath(folderPath));
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

    QFile file(ProjectPaths::projectJsonPath(folderPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return false;
    }

    if (!document.isObject()) {
        return false;
    }

    const QJsonObject jsonObj = document.object();
    if (jsonObj.value(QStringLiteral("projectType")).toString()
        != AppInfo::ProjectTypeId) {
        return false;
    }

    const int schemaVersion = jsonObj.value(QStringLiteral("schemaVersion")).toInt(-1);
    if (schemaVersion != 1) {
        return false;
    }

    config.projectName = dir.dirName();
    config.projectPath = folderPath;
    config.projectType = jsonObj.value(QStringLiteral("projectType"))
        .toString(AppInfo::ProjectTypeId);
    config.createdDate = jsonObj.value(QStringLiteral("createdDate")).toString();
    config.schemaVersion = schemaVersion;
    return true;
}
