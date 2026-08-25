#include "SimulationResultService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

bool readFlagSuccess(const QString &flagPath)
{
    QFile flagFile(flagPath);
    if (!flagFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QString content =
        QString::fromUtf8(flagFile.readAll()).trimmed();
    return content == QStringLiteral("success");
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

    const bool t1Finished =
        readFlagSuccess(ProjectInputHash::t1FinishedFlagPath(projectPath));
    const bool odbExists =
        QFile::exists(ProjectInputHash::solverOdbPath(projectPath));
    const bool t2Finished =
        readFlagSuccess(ProjectInputHash::t2FinishedFlagPath(projectPath));

    if (!t2Finished) {
        if (t1Finished && odbExists) {
            result.state = ResultValidationState::PostIncomplete;
            result.message =
                QStringLiteral(
                    "Abaqus 求解已经完成，但后处理尚未完整完成。"
                    "请继续后处理后再查看结果。"
                );
        } else {
            result.state = ResultValidationState::NoResults;
            result.message =
                QStringLiteral(
                    "当前工程尚无完整仿真结果。"
                    "请先完成 Abaqus 仿真及后处理。"
                );
        }
        return result;
    }

    result.manifest =
        ProjectInputHash::readPostProcessManifest(projectPath);
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

QVector<ResultTypeDescriptor> SimulationResultService::allResultTypes()
{
    return {
        descriptorFor(ResultType::Cure),
        descriptorFor(ResultType::Temperature),
        descriptorFor(ResultType::Stress),
    };
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

bool SimulationResultService::readSuccessFlag(const QString &flagPath)
{
    return readFlagSuccess(flagPath);
}

bool SimulationResultService::isReportCurrent(const QString &projectPath)
{
    const ResultValidationResult validation = validate(projectPath);
    if (!validation.isValid()) {
        return false;
    }

    const QString manifestPath = reportManifestPath(projectPath);
    QFile file(manifestPath);
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
    if (json.value(QStringLiteral("version")).toInt() != 1) {
        return false;
    }

    const QString storedSha =
        json.value(QStringLiteral("postSha256")).toString();
    const QFileInfo pdfInfo(reportPdfPath(projectPath));
    return !storedSha.isEmpty()
        && storedSha == validation.manifest.postSha256
        && pdfInfo.exists()
        && pdfInfo.size() > 0;
}
