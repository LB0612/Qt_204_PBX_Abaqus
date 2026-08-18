#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>

struct ProjectConfig
{
    QString projectName;
    QString projectPath;

    QString projectType = QStringLiteral("PBX_CASTING_CURING");

    QString createdDate;

    QString softwareVersion = QStringLiteral("1.0");

    int schemaVersion = 1;
};

class ProjectManager
{
public:
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

    static bool isValidProject(
        const QString &folderPath
    );
};

#endif // PROJECTMANAGER_H
