#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "AppInfo.h"

#include <QString>

struct ProjectConfig
{
    QString projectName;
    QString projectPath;

    QString projectType = AppInfo::ProjectTypeId;

    QString createdDate;

    int schemaVersion = 1;
};

class ProjectManager
{
public:
    static bool isValidProjectName(
        const QString &projectName,
        QString &errorMessage
    );

    static bool createProject(
        const QString &basePath,
        const QString &projectName,
        ProjectConfig &config,
        QString &errorMessage
    );

    static bool saveProject(
        const QString &folderPath,
        const ProjectConfig &config
    );

    static bool loadProject(
        const QString &folderPath,
        ProjectConfig &config
    );
};

#endif // PROJECTMANAGER_H
