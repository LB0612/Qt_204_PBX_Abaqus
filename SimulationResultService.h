#ifndef SIMULATIONRESULTSERVICE_H
#define SIMULATIONRESULTSERVICE_H

#include "ProjectInputHash.h"

#include <QString>

enum class ResultType
{
    Cure,
    Temperature,
    Stress
};

enum class ResultValidationState
{
    Valid,
    ProjectMissing,
    NoResults,
    PostIncomplete,
    ManifestInvalid,
    PostShaMismatch,
    OutputsInvalid
};

struct ResultValidationResult
{
    ResultValidationState state = ResultValidationState::ProjectMissing;
    QString message;
    ProjectInputHash::PostProcessManifest manifest;

    bool isValid() const
    {
        return state == ResultValidationState::Valid;
    }
};

struct ResultTypeDescriptor
{
    ResultType type = ResultType::Cure;
    QString displayName;
    QString resultsBaseName;
    QString framePrefix;
};

class SimulationResultService
{
public:
    static ResultValidationResult validate(const QString &projectPath);

    static ResultTypeDescriptor descriptorFor(ResultType type);

    static QString frameDirectory(
        const QString &projectPath,
        ResultType type);
    static QString framePngPath(
        const QString &projectPath,
        ResultType type,
        int frameIndex);
    static QString aviPath(
        const QString &projectPath,
        ResultType type);
    static QString resultsDirectoryPath(const QString &projectPath);
    static QString reportDirectoryPath(const QString &projectPath);
    static QString reportPdfPath(const QString &projectPath);
    static QString reportDocxPath(const QString &projectPath);
    static QString reportManifestPath(const QString &projectPath);

    static bool isReportCurrent(const QString &projectPath);
};

#endif
