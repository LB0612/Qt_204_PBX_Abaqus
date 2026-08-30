#include "SimulationReportGenerator.h"

#include "AppInfo.h"
#include "ProjectInputHash.h"
#include "ProjectManager.h"
#include "ProjectParameterService.h"
#include "SimulationArtifactStateService.h"
#include "SimulationReportDocxWriter.h"
#include "SimulationReportModel.h"
#include "SimulationReportPdfWriter.h"
#include "SimulationResultService.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QVector>

namespace {

QString formatNumber(double value)
{
    return QString::number(value, 'g', 15);
}

struct RepresentativeFrame
{
    int index = 0;
    QString label;
};

QVector<RepresentativeFrame> representativeFrames(int frameCount)
{
    QVector<RepresentativeFrame> result;

    if (frameCount <= 0) {
        return result;
    }

    if (frameCount == 1) {
        result.append({0, QStringLiteral("结果时刻")});
        return result;
    }

    if (frameCount == 2) {
        result.append({0, QStringLiteral("初始时刻")});
        result.append({1, QStringLiteral("最终时刻")});
        return result;
    }

    result.append({0, QStringLiteral("初始时刻")});
    result.append({(frameCount - 1) / 2, QStringLiteral("中间时刻")});
    result.append({frameCount - 1, QStringLiteral("最终时刻")});
    return result;
}

bool buildResultSection(
    const QString &projectPath,
    const ProjectInputHash::PostProcessManifest &manifest,
    ResultType type,
    const QString &title,
    SimulationReportResultSection &section,
    QString &errorMessage)
{
    section = SimulationReportResultSection();
    section.title = title;

    const int frameCount = manifest.odbFrames;
    const QVector<RepresentativeFrame> frames =
        representativeFrames(frameCount);

    for (const RepresentativeFrame &frame : frames) {
        const QString imagePath =
            SimulationResultService::framePngPath(
                projectPath,
                type,
                frame.index
            );

        QImageReader reader(imagePath);
        if (!reader.canRead()) {
            errorMessage =
                QStringLiteral("无法读取报告代表帧：\n%1")
                    .arg(imagePath);
            return false;
        }

        const QSize imageSize = reader.size();
        if (!imageSize.isValid()) {
            errorMessage =
                QStringLiteral("无法取得报告图片尺寸：\n%1")
                    .arg(imagePath);
            return false;
        }

        SimulationReportFigure figure;
        figure.label = frame.label;
        figure.frameText =
            QStringLiteral("Frame %1 / %2")
                .arg(frame.index + 1)
                .arg(frameCount);

        if (manifest.frameTimes.size() == frameCount) {
            figure.timeText =
                QStringLiteral("仿真时间：%1 s")
                    .arg(
                        QString::number(
                            manifest.frameTimes.at(frame.index),
                            'f',
                            2
                        )
                    );
        }

        figure.imagePath = imagePath;
        figure.imagePixelSize = imageSize;
        section.figures.append(figure);
    }

    return true;
}

bool buildReportModel(
    const QString &projectPath,
    const ResultValidationResult &validation,
    SimulationReportModel &model,
    QString &errorMessage)
{
    ProjectConfig project;
    if (!ProjectManager::loadProject(projectPath, project)) {
        errorMessage = QStringLiteral("无法读取工程信息。");
        return false;
    }

    ProjectParameters parameters;
    if (!ProjectParameterService::loadAll(
            projectPath,
            parameters,
            errorMessage)) {
        return false;
    }

    model = SimulationReportModel();
    model.reportTitle =
        QStringLiteral("浇注PBX固化仿真分析报告");
    model.productName = AppInfo::ProductName;
    model.appVersion =
        QCoreApplication::applicationVersion();
    model.projectName = project.projectName;
    model.jobName =
        ProjectInputHash::currentJobName(projectPath);
    model.generatedAt =
        QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss")
        );
    model.t2CompletedAt =
        SimulationArtifactStateService::t2CompletionStampUtc(
            projectPath
        );

    if (model.t2CompletedAt.isEmpty()) {
        errorMessage =
            QStringLiteral("无法取得后处理完成时间。");
        return false;
    }

    model.postSha256 = validation.manifest.postSha256;

    QString actualTimeRange = QStringLiteral("未记录");
    QString actualTimeUnit;
    if (validation.manifest.odbFrames > 0
        && validation.manifest.frameTimes.size()
            == validation.manifest.odbFrames) {
        actualTimeRange =
            QStringLiteral("%1 ～ %2")
                .arg(
                    QString::number(
                        validation.manifest.frameTimes.first(),
                        'f',
                        2
                    )
                )
                .arg(
                    QString::number(
                        validation.manifest.frameTimes.last(),
                        'f',
                        2
                    )
                );
        actualTimeUnit = QStringLiteral("s");
    }

    model.overviewRows = {
        {
            QStringLiteral("工程名称"),
            model.projectName,
            QString()
        },
        {
            QStringLiteral("Abaqus Job 名称"),
            model.jobName,
            QString()
        },
        {
            QStringLiteral("设定仿真时长"),
            formatNumber(parameters.simulation.timeLength),
            QStringLiteral("s")
        },
        {
            QStringLiteral("实际结果时间范围"),
            actualTimeRange,
            actualTimeUnit
        },
        {
            QStringLiteral("ODB总帧数"),
            QString::number(validation.manifest.odbFrames),
            QStringLiteral("帧")
        },
        {
            QStringLiteral("固化度结果帧数"),
            QString::number(validation.manifest.curePngFrames),
            QStringLiteral("帧")
        },
        {
            QStringLiteral("温度场结果帧数"),
            QString::number(
                validation.manifest.temperaturePngFrames
            ),
            QStringLiteral("帧")
        },
        {
            QStringLiteral("Mises应力结果帧数"),
            QString::number(validation.manifest.stressPngFrames),
            QStringLiteral("帧")
        },
        {
            QStringLiteral("视频帧率"),
            QString::number(validation.manifest.videoFps),
            QStringLiteral("fps")
        }
    };

    {
        SimulationReportTable structureTable;
        structureTable.title = QStringLiteral("结构参数");
        structureTable.rows = {
            {
                QStringLiteral("药柱半径"),
                formatNumber(parameters.structure.chargeRadius),
                QStringLiteral("mm")
            },
            {
                QStringLiteral("药柱高度"),
                formatNumber(parameters.structure.chargeHeight),
                QStringLiteral("mm")
            },
            {
                QStringLiteral("外壳厚度"),
                formatNumber(parameters.structure.shellThickness),
                QStringLiteral("mm")
            }
        };
        model.parameterTables.append(structureTable);
    }

    {
        SimulationReportTable explosiveTable;
        explosiveTable.title = QStringLiteral("炸药参数");
        explosiveTable.rows = {
            {
                QStringLiteral("密度"),
                formatNumber(parameters.explosive.density),
                QStringLiteral("t/mm³")
            },
            {
                QStringLiteral("初始杨氏模量"),
                formatNumber(
                    parameters.explosive.initialElasticModulus
                ),
                QStringLiteral("MPa")
            },
            {
                QStringLiteral("初始泊松比"),
                formatNumber(
                    parameters.explosive.initialPoissonRatio
                ),
                QStringLiteral("—")
            },
            {
                QStringLiteral("最终杨氏模量"),
                formatNumber(
                    parameters.explosive.finalElasticModulus
                ),
                QStringLiteral("MPa")
            },
            {
                QStringLiteral("最终泊松比"),
                formatNumber(
                    parameters.explosive.finalPoissonRatio
                ),
                QStringLiteral("—")
            },
            {
                QStringLiteral("热导率"),
                formatNumber(
                    parameters.explosive.thermalConductivity
                ),
                QStringLiteral("W/(m·K)")
            },
            {
                QStringLiteral("屈服应力"),
                formatNumber(parameters.explosive.yieldStress),
                QStringLiteral("MPa")
            },
            {
                QStringLiteral("比热"),
                formatNumber(parameters.explosive.specificHeat),
                QStringLiteral("N·mm/(t·K)")
            },
            {
                QStringLiteral("膨胀系数"),
                formatNumber(
                    parameters.explosive.expansionCoefficient
                ),
                QStringLiteral("1/K")
            }
        };
        model.parameterTables.append(explosiveTable);
    }

    {
        SimulationReportTable moldTable;
        moldTable.title = QStringLiteral("模具参数");
        moldTable.rows = {
            {
                QStringLiteral("密度"),
                formatNumber(parameters.mold.density),
                QStringLiteral("t/mm³")
            },
            {
                QStringLiteral("杨氏模量"),
                formatNumber(parameters.mold.elasticModulus),
                QStringLiteral("MPa")
            },
            {
                QStringLiteral("泊松比"),
                formatNumber(parameters.mold.poissonRatio),
                QStringLiteral("—")
            },
            {
                QStringLiteral("热导率"),
                formatNumber(parameters.mold.thermalConductivity),
                QStringLiteral("W/(m·K)")
            },
            {
                QStringLiteral("比热"),
                formatNumber(parameters.mold.specificHeat),
                QStringLiteral("N·mm/(t·K)")
            }
        };
        model.parameterTables.append(moldTable);
    }

    {
        SimulationReportTable boundaryTable;
        boundaryTable.title = QStringLiteral("边界条件");
        boundaryTable.rows = {
            {
                QStringLiteral("环境温度"),
                formatNumber(
                    parameters.boundary.ambientTemperature
                ),
                QStringLiteral("K")
            }
        };
        model.parameterTables.append(boundaryTable);
    }

    {
        SimulationReportTable simulationTable;
        simulationTable.title = QStringLiteral("仿真参数");
        simulationTable.rows = {
            {
                QStringLiteral("仿真时间长度"),
                formatNumber(parameters.simulation.timeLength),
                QStringLiteral("s")
            }
        };
        model.parameterTables.append(simulationTable);
    }

    SimulationReportResultSection cure;
    if (!buildResultSection(
            projectPath,
            validation.manifest,
            ResultType::Cure,
            QStringLiteral("固化度结果"),
            cure,
            errorMessage)) {
        return false;
    }

    SimulationReportResultSection temperature;
    if (!buildResultSection(
            projectPath,
            validation.manifest,
            ResultType::Temperature,
            QStringLiteral("温度场结果"),
            temperature,
            errorMessage)) {
        return false;
    }

    SimulationReportResultSection stress;
    if (!buildResultSection(
            projectPath,
            validation.manifest,
            ResultType::Stress,
            QStringLiteral("Mises应力结果"),
            stress,
            errorMessage)) {
        return false;
    }

    model.resultSections = {cure, temperature, stress};

    model.notes = {
        QStringLiteral("本报告展示当前工程代表性结果帧。"),
        QStringLiteral("固化度完整动态结果：guhuadu.avi"),
        QStringLiteral("温度场完整动态结果：wendu.avi"),
        QStringLiteral("Mises应力完整动态结果：yingli.avi"),
        QStringLiteral(
            "完整动态过程请查看工程 results 目录。"
        )
    };

    model.traceRows = {
        {
            QStringLiteral("软件版本"),
            model.appVersion,
            QString()
        },
        {
            QStringLiteral("报告格式版本"),
            QString::number(model.reportFormatVersion),
            QString()
        },
        {
            QStringLiteral("后处理SHA256"),
            model.postSha256,
            QString()
        },
        {
            QStringLiteral("后处理完成时间"),
            model.t2CompletedAt,
            QStringLiteral("UTC")
        },
        {
            QStringLiteral("报告生成时间"),
            model.generatedAt,
            QString()
        }
    };

    return true;
}

bool writeReportManifest(
    const QString &projectPath,
    const SimulationReportModel &model,
    const QString &pdfPath,
    const QString &docxPath,
    const QString &pdfSha256,
    const QString &docxSha256,
    QString &errorMessage)
{
    const QFileInfo pdfInfo(pdfPath);
    const QFileInfo docxInfo(docxPath);

    const QJsonObject json = {
        {QStringLiteral("version"), 3},
        {QStringLiteral("postSha256"), model.postSha256},
        {QStringLiteral("t2CompletedAt"), model.t2CompletedAt},
        {QStringLiteral("generatedAt"), model.generatedAt},
        {QStringLiteral("pdf"), pdfInfo.fileName()},
        {QStringLiteral("docx"), docxInfo.fileName()},
        {
            QStringLiteral("pdfBytes"),
            static_cast<double>(pdfInfo.size())
        },
        {
            QStringLiteral("docxBytes"),
            static_cast<double>(docxInfo.size())
        },
        {QStringLiteral("pdfSha256"), pdfSha256},
        {QStringLiteral("docxSha256"), docxSha256}
    };

    QSaveFile file(
        SimulationResultService::reportManifestPath(projectPath)
    );
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errorMessage = QStringLiteral("无法写入报告清单。");
        return false;
    }

    file.write(
        QJsonDocument(json).toJson(QJsonDocument::Indented)
    );

    if (!file.commit()) {
        errorMessage = QStringLiteral("报告清单保存失败。");
        return false;
    }

    return true;
}

int expectedBodyPageCount(
    const SimulationReportModel &model)
{
    int figurePages = 0;
    for (const SimulationReportResultSection &section
         : model.resultSections) {
        figurePages += section.figures.size();
    }

    // 1 overview
    // 2 parameter pages
    // N figure pages
    // 1 notes page
    // 1 trace page
    return figurePages + 5;
}

bool validatePdfFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    return file.size() > 0
        && file.read(5) == QByteArray("%PDF-");
}

bool validateDocxFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray signature = file.read(4);
    return file.size() > 0
        && signature.size() == 4
        && signature.at(0) == 'P'
        && signature.at(1) == 'K'
        && static_cast<unsigned char>(signature.at(2)) == 0x03
        && static_cast<unsigned char>(signature.at(3)) == 0x04;
}

void restoreFromBackup(
    const QString &finalPath,
    const QString &backupPath,
    bool hadBackup)
{
    if (QFile::exists(finalPath)) {
        QFile::remove(finalPath);
    }
    if (hadBackup) {
        QFile::rename(backupPath, finalPath);
    }
}

void cleanupBackup(const QString &backupPath, bool hadBackup)
{
    if (hadBackup && QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
}

bool reportFinalsMatchManifest(const QString &projectPath)
{
    const QString pdfPath =
        SimulationResultService::reportPdfPath(projectPath);
    const QString docxPath =
        SimulationResultService::reportDocxPath(projectPath);

    QFile file(
        SimulationResultService::reportManifestPath(projectPath)
    );
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
    if (json.value(QStringLiteral("version")).toInt() != 3) {
        return false;
    }

    const QString storedPdfName =
        json.value(QStringLiteral("pdf")).toString();
    const QString storedDocxName =
        json.value(QStringLiteral("docx")).toString();
    const qint64 storedPdfBytes =
        static_cast<qint64>(
            json.value(QStringLiteral("pdfBytes")).toDouble(-1)
        );
    const qint64 storedDocxBytes =
        static_cast<qint64>(
            json.value(QStringLiteral("docxBytes")).toDouble(-1)
        );
    const QString storedPdfSha =
        json.value(QStringLiteral("pdfSha256")).toString();
    const QString storedDocxSha =
        json.value(QStringLiteral("docxSha256")).toString();

    if (storedPdfName != QFileInfo(pdfPath).fileName()
        || storedDocxName != QFileInfo(docxPath).fileName()
        || storedPdfSha.isEmpty()
        || storedDocxSha.isEmpty()) {
        return false;
    }

    const QFileInfo pdfInfo(pdfPath);
    const QFileInfo docxInfo(docxPath);
    if (!pdfInfo.exists()
        || !pdfInfo.isFile()
        || pdfInfo.size() <= 0
        || !docxInfo.exists()
        || !docxInfo.isFile()
        || docxInfo.size() <= 0
        || pdfInfo.size() != storedPdfBytes
        || docxInfo.size() != storedDocxBytes) {
        return false;
    }

    const QString actualPdfSha =
        SimulationArtifactStateService::fileSha256(pdfPath);
    const QString actualDocxSha =
        SimulationArtifactStateService::fileSha256(docxPath);

    return !actualPdfSha.isEmpty()
        && !actualDocxSha.isEmpty()
        && actualPdfSha == storedPdfSha
        && actualDocxSha == storedDocxSha;
}

void recoverInterruptedReportCommit(
    const QString &pdfPath,
    const QString &docxPath,
    const QString &backupPdfPath,
    const QString &backupDocxPath,
    const QString &projectPath)
{
    const bool hasPdfBak = QFile::exists(backupPdfPath);
    const bool hasDocxBak = QFile::exists(backupDocxPath);
    if (!hasPdfBak && !hasDocxBak) {
        return;
    }

    if (reportFinalsMatchManifest(projectPath)) {
        // Manifest already committed; only leftover .bak cleanup.
        if (hasPdfBak) {
            QFile::remove(backupPdfPath);
        }
        if (hasDocxBak) {
            QFile::remove(backupDocxPath);
        }
        return;
    }

    // Crash between promoting finals and writing manifest:
    // drop residual new finals and restore the last known-good pair.
    if (hasPdfBak) {
        if (QFile::exists(pdfPath)) {
            QFile::remove(pdfPath);
        }
        QFile::rename(backupPdfPath, pdfPath);
    }
    if (hasDocxBak) {
        if (QFile::exists(docxPath)) {
            QFile::remove(docxPath);
        }
        QFile::rename(backupDocxPath, docxPath);
    }
}

} // namespace

bool SimulationReportGenerator::generate(
    const QString &projectPath,
    QString &errorMessage)
{
    const ResultValidationResult validation =
        SimulationResultService::validate(projectPath);
    if (!validation.isValid()) {
        errorMessage = validation.message;
        return false;
    }

    SimulationReportModel model;
    if (!buildReportModel(
            projectPath,
            validation,
            model,
            errorMessage)) {
        return false;
    }

    const QString reportDir =
        SimulationResultService::reportDirectoryPath(projectPath);
    if (!QDir().mkpath(reportDir)) {
        errorMessage = QStringLiteral("无法创建报告目录。");
        return false;
    }

    const QString pdfPath =
        SimulationResultService::reportPdfPath(projectPath);
    const QString docxPath =
        SimulationResultService::reportDocxPath(projectPath);
    const QString tempPdfPath =
        pdfPath + QStringLiteral(".tmp");
    const QString tempDocxPath =
        docxPath + QStringLiteral(".tmp");
    const QString backupPdfPath =
        pdfPath + QStringLiteral(".bak");
    const QString backupDocxPath =
        docxPath + QStringLiteral(".bak");

    recoverInterruptedReportCommit(
        pdfPath,
        docxPath,
        backupPdfPath,
        backupDocxPath,
        projectPath
    );

    if (QFile::exists(tempPdfPath)) {
        QFile::remove(tempPdfPath);
    }
    if (QFile::exists(tempDocxPath)) {
        QFile::remove(tempDocxPath);
    }

    int bodyTotalPages = 0;
    if (!SimulationReportPdfWriter::write(
            model,
            tempPdfPath,
            errorMessage,
            &bodyTotalPages)) {
        if (QFile::exists(tempPdfPath)) {
            QFile::remove(tempPdfPath);
        }
        return false;
    }

    const int expectedPages = expectedBodyPageCount(model);
    if (bodyTotalPages != expectedPages) {
        QFile::remove(tempPdfPath);
        errorMessage =
            QStringLiteral(
                "报告分页与预期不一致："
                "实际 %1 页，预期 %2 页。"
            )
                .arg(bodyTotalPages)
                .arg(expectedPages);
        return false;
    }

    if (!SimulationReportDocxWriter::write(
            model,
            tempDocxPath,
            errorMessage,
            bodyTotalPages)) {
        if (QFile::exists(tempPdfPath)) {
            QFile::remove(tempPdfPath);
        }
        if (QFile::exists(tempDocxPath)) {
            QFile::remove(tempDocxPath);
        }
        return false;
    }

    if (!validatePdfFile(tempPdfPath)) {
        QFile::remove(tempPdfPath);
        QFile::remove(tempDocxPath);
        errorMessage = QStringLiteral("PDF 报告校验失败。");
        return false;
    }

    if (!validateDocxFile(tempDocxPath)) {
        QFile::remove(tempPdfPath);
        QFile::remove(tempDocxPath);
        errorMessage = QStringLiteral("Word 报告校验失败。");
        return false;
    }

    const QString pdfSha256 =
        SimulationArtifactStateService::fileSha256(tempPdfPath);
    const QString docxSha256 =
        SimulationArtifactStateService::fileSha256(tempDocxPath);

    if (pdfSha256.isEmpty() || docxSha256.isEmpty()) {
        QFile::remove(tempPdfPath);
        QFile::remove(tempDocxPath);
        errorMessage =
            QStringLiteral("无法计算报告文件校验值。");
        return false;
    }

    bool oldPdfBackedUp = false;
    bool oldDocxBackedUp = false;

    if (QFile::exists(pdfPath)) {
        if (!QFile::rename(pdfPath, backupPdfPath)) {
            QFile::remove(tempPdfPath);
            QFile::remove(tempDocxPath);
            errorMessage =
                QStringLiteral(
                    "无法暂存旧 PDF 报告，"
                    "文件可能正在被占用：\n%1"
                ).arg(pdfPath);
            return false;
        }
        oldPdfBackedUp = true;
    }

    if (QFile::exists(docxPath)) {
        if (!QFile::rename(docxPath, backupDocxPath)) {
            QFile::remove(tempPdfPath);
            QFile::remove(tempDocxPath);
            restoreFromBackup(
                pdfPath,
                backupPdfPath,
                oldPdfBackedUp
            );
            errorMessage =
                QStringLiteral(
                    "无法暂存旧 Word 报告，"
                    "文件可能正在被占用：\n%1"
                ).arg(docxPath);
            return false;
        }
        oldDocxBackedUp = true;
    }

    if (!QFile::rename(tempPdfPath, pdfPath)) {
        QFile::remove(tempPdfPath);
        QFile::remove(tempDocxPath);
        restoreFromBackup(pdfPath, backupPdfPath, oldPdfBackedUp);
        restoreFromBackup(
            docxPath,
            backupDocxPath,
            oldDocxBackedUp
        );
        errorMessage =
            QStringLiteral("无法提交正式 PDF 报告文件。");
        return false;
    }

    if (!QFile::rename(tempDocxPath, docxPath)) {
        QFile::remove(tempDocxPath);
        // Critical: never leave new PDF + old DOCX.
        restoreFromBackup(pdfPath, backupPdfPath, oldPdfBackedUp);
        restoreFromBackup(
            docxPath,
            backupDocxPath,
            oldDocxBackedUp
        );
        errorMessage =
            QStringLiteral("无法提交正式 Word 报告文件。");
        return false;
    }

    if (!writeReportManifest(
            projectPath,
            model,
            pdfPath,
            docxPath,
            pdfSha256,
            docxSha256,
            errorMessage)) {
        restoreFromBackup(pdfPath, backupPdfPath, oldPdfBackedUp);
        restoreFromBackup(
            docxPath,
            backupDocxPath,
            oldDocxBackedUp
        );
        return false;
    }

    cleanupBackup(backupPdfPath, oldPdfBackedUp);
    cleanupBackup(backupDocxPath, oldDocxBackedUp);
    return true;
}
