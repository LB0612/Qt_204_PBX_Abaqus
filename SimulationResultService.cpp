#include "SimulationResultService.h"

#include "SimulationArtifactStateService.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString t2CompletionStamp(const QString &projectPath)
{
    const QFileInfo info(
        ProjectInputHash::t2FinishedFlagPath(projectPath)
    );

    if (!info.exists() || !info.isFile()) {
        return QString();
    }

    return info.lastModified().toUTC().toString(Qt::ISODateWithMs);
}

} // namespace

ResultValidationResult SimulationResultService::validate(
    const QString &projectPath)
{
    ResultValidationResult result;

    if (projectPath.isEmpty()
        || !QFileInfo(projectPath).exists()) {
        result.state = ResultValidationState::ProjectMissing;
        result.message =
            QStringLiteral("当前工程目录不存在或无效。");
        return result;
    }

    const bool solverResultValid =
        SimulationArtifactStateService::hasValidSolverResult(
            projectPath
        );

    const bool t2Finished =
        SimulationArtifactStateService::readSuccessFlag(
            ProjectInputHash::t2FinishedFlagPath(projectPath)
        );

    if (!t2Finished) {
        if (solverResultValid) {
            result.state = ResultValidationState::PostIncomplete;
            result.message =
                QStringLiteral(
                    "Abaqus 求解已经完成，"
                    "但后处理尚未完整完成。"
                    "可以继续后处理，"
                    "也可以选择重新完整仿真。"
                );
        } else {
            result.state = ResultValidationState::NoResults;
            result.message =
                QStringLiteral(
                    "当前工程没有可确认有效的完整求解结果。"
                    "请重新进行仿真。"
                );
        }
        return result;
    }

    if (!solverResultValid) {
        result.state = ResultValidationState::PostShaMismatch;
        result.message =
            QStringLiteral(
                "当前 Abaqus 求解结果状态无效"
                "或与当前输入不一致。"
                "请重新进行完整仿真。"
            );
        return result;
    }

    result.manifest =
        SimulationArtifactStateService::postProcessManifest(
            projectPath
        );
    if (!result.manifest.valid) {
        result.state = ResultValidationState::ManifestInvalid;
        result.message =
            QStringLiteral("后处理清单无效或版本不匹配。");
        return result;
    }

    const QString currentPostSha =
        ProjectInputHash::hashPostProcessInput(projectPath);
    if (currentPostSha.isEmpty()
        || currentPostSha != result.manifest.postSha256) {
        result.state = ResultValidationState::PostShaMismatch;
        result.message =
            QStringLiteral(
                "当前工程参数与现有仿真结果不一致。"
                "请重新生成文件并完成仿真后再查看结果。"
            );
        return result;
    }

    QString outputError;
    if (!ProjectInputHash::validatePostProcessOutputs(
            projectPath,
            outputError)) {
        result.state = ResultValidationState::OutputsInvalid;
        result.message = outputError.isEmpty()
            ? QStringLiteral("仿真结果文件不完整或已损坏。")
            : outputError;
        return result;
    }

    result.state = ResultValidationState::Valid;
    return result;
}

ResultTypeDescriptor SimulationResultService::descriptorFor(
    ResultType type)
{
    switch (type) {
    case ResultType::Cure:
        return {
            ResultType::Cure,
            QStringLiteral("固化度"),
            QStringLiteral("guhuadu"),
            QStringLiteral("Cure_SDV1_frame"),
        };
    case ResultType::Temperature:
        return {
            ResultType::Temperature,
            QStringLiteral("温度场"),
            QStringLiteral("wendu"),
            QStringLiteral("NT11_frame"),
        };
    case ResultType::Stress:
        return {
            ResultType::Stress,
            QStringLiteral("Mises应力场"),
            QStringLiteral("yingli"),
            QStringLiteral("Stress_Mises_frame"),
        };
    }

    return descriptorFor(ResultType::Cure);
}

QString SimulationResultService::frameDirectory(
    const QString &projectPath,
    ResultType type)
{
    const ResultTypeDescriptor desc = descriptorFor(type);
    return QDir(resultsDirectoryPath(projectPath))
        .filePath(desc.resultsBaseName + QStringLiteral("_frames"));
}

QString SimulationResultService::framePngPath(
    const QString &projectPath,
    ResultType type,
    int frameIndex)
{
    const ResultTypeDescriptor desc = descriptorFor(type);
    return QDir(frameDirectory(projectPath, type)).filePath(
        QStringLiteral("%1_%2.png")
            .arg(desc.framePrefix)
            .arg(frameIndex, 8, 10, QLatin1Char('0'))
    );
}

QString SimulationResultService::aviPath(
    const QString &projectPath,
    ResultType type)
{
    const ResultTypeDescriptor desc = descriptorFor(type);
    return QDir(resultsDirectoryPath(projectPath))
        .filePath(desc.resultsBaseName + QStringLiteral(".avi"));
}

QString SimulationResultService::resultsDirectoryPath(
    const QString &projectPath)
{
    return ProjectInputHash::resultsDirectory(projectPath);
}

QString SimulationResultService::reportDirectoryPath(
    const QString &projectPath)
{
    return QDir(resultsDirectoryPath(projectPath))
        .filePath(QStringLiteral("report"));
}

QString SimulationResultService::reportPdfPath(
    const QString &projectPath)
{
    return QDir(reportDirectoryPath(projectPath))
        .filePath(QStringLiteral("simulation_report.pdf"));
}

QString SimulationResultService::reportManifestPath(
    const QString &projectPath)
{
    return QDir(reportDirectoryPath(projectPath))
        .filePath(QStringLiteral("report_manifest.json"));
}

bool SimulationResultService::isReportCurrent(const QString &projectPath)
{
    const ProjectInputHash::PostProcessManifest postManifest =
        SimulationArtifactStateService::postProcessManifest(
            projectPath
        );

    if (!postManifest.valid) {
        return false;
    }

    const QString currentPostSha =
        ProjectInputHash::hashPostProcessInput(projectPath);

    if (currentPostSha.isEmpty()
        || currentPostSha != postManifest.postSha256) {
        return false;
    }

    const QString currentT2Stamp = t2CompletionStamp(projectPath);
    if (currentT2Stamp.isEmpty()) {
        return false;
    }

    QFile file(reportManifestPath(projectPath));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);

    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        return false;
    }

    const QJsonObject json = document.object();
    if (json.value(QStringLiteral("version")).toInt() != 2) {
        return false;
    }

    const QString storedSha =
        json.value(QStringLiteral("postSha256")).toString();
    const QString storedT2Stamp =
        json.value(QStringLiteral("t2CompletedAt")).toString();

    const QFileInfo pdfInfo(reportPdfPath(projectPath));

    return !storedSha.isEmpty()
        && storedSha == postManifest.postSha256
        && storedT2Stamp == currentT2Stamp
        && pdfInfo.exists()
        && pdfInfo.isFile()
        && pdfInfo.size() > 0;
}
