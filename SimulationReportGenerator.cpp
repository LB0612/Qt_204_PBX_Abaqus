#include "SimulationReportGenerator.h"

#include "BoundaryConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "ProjectInputHash.h"
#include "ProjectManager.h"
#include "SimulationConfigManager.h"
#include "SimulationResultService.h"
#include "StructureConfigManager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPageLayout>
#include <QPageSize>
#include <QMarginsF>
#include <QPdfWriter>
#include <QSaveFile>
#include <QTextDocument>
#include <QUrl>

namespace {

QString htmlEscape(const QString &text)
{
    QString escaped = text;
    escaped.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    escaped.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    escaped.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    return escaped;
}

QString formatDouble(double value, int precision = 4)
{
    return QString::number(value, 'f', precision);
}

QString buildParameterSectionHtml(const QString &projectPath)
{
    StructureConfig structure;
    ExplosiveConfig explosive;
    MoldConfig mold;
    BoundaryConfig boundary;
    SimulationConfig simulation;

    StructureConfigManager::load(projectPath, structure);
    ExplosiveConfigManager::load(projectPath, explosive);
    MoldConfigManager::load(projectPath, mold);
    BoundaryConfigManager::load(projectPath, boundary);
    SimulationConfigManager::load(projectPath, simulation);

    QString html;
    html += QStringLiteral("<h2>输入参数</h2>");

    html += QStringLiteral("<h3>结构参数</h3><table border='1' cellspacing='0' cellpadding='6'>");
    html += QStringLiteral("<tr><th>参数</th><th>数值</th><th>单位</th></tr>");
    html += QStringLiteral("<tr><td>药柱半径</td><td>%1</td><td>mm</td></tr>")
        .arg(formatDouble(structure.chargeRadius));
    html += QStringLiteral("<tr><td>药柱高度</td><td>%1</td><td>mm</td></tr>")
        .arg(formatDouble(structure.chargeHeight));
    html += QStringLiteral("<tr><td>外壳厚度</td><td>%1</td><td>mm</td></tr>")
        .arg(formatDouble(structure.shellThickness));
    html += QStringLiteral("</table>");

    html += QStringLiteral("<h3>炸药参数</h3><table border='1' cellspacing='0' cellpadding='6'>");
    html += QStringLiteral("<tr><th>参数</th><th>数值</th></tr>");
    html += QStringLiteral("<tr><td>密度</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.density, 6));
    html += QStringLiteral("<tr><td>初始杨氏模量</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.initialElasticModulus));
    html += QStringLiteral("<tr><td>初始泊松比</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.initialPoissonRatio));
    html += QStringLiteral("<tr><td>最终杨氏模量</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.finalElasticModulus));
    html += QStringLiteral("<tr><td>最终泊松比</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.finalPoissonRatio));
    html += QStringLiteral("<tr><td>热导率</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.thermalConductivity));
    html += QStringLiteral("<tr><td>屈服应力</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.yieldStress));
    html += QStringLiteral("<tr><td>比热</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.specificHeat, 2));
    html += QStringLiteral("<tr><td>膨胀系数</td><td>%1</td></tr>")
        .arg(formatDouble(explosive.expansionCoefficient, 8));
    html += QStringLiteral("</table>");

    html += QStringLiteral("<h3>模具参数</h3><table border='1' cellspacing='0' cellpadding='6'>");
    html += QStringLiteral("<tr><th>参数</th><th>数值</th></tr>");
    html += QStringLiteral("<tr><td>密度</td><td>%1</td></tr>")
        .arg(formatDouble(mold.density, 6));
    html += QStringLiteral("<tr><td>杨氏模量</td><td>%1</td></tr>")
        .arg(formatDouble(mold.elasticModulus));
    html += QStringLiteral("<tr><td>泊松比</td><td>%1</td></tr>")
        .arg(formatDouble(mold.poissonRatio));
    html += QStringLiteral("<tr><td>热导率</td><td>%1</td></tr>")
        .arg(formatDouble(mold.thermalConductivity));
    html += QStringLiteral("<tr><td>比热</td><td>%1</td></tr>")
        .arg(formatDouble(mold.specificHeat, 2));
    html += QStringLiteral("</table>");

    html += QStringLiteral("<h3>边界条件</h3><table border='1' cellspacing='0' cellpadding='6'>");
    html += QStringLiteral("<tr><th>参数</th><th>数值</th></tr>");
    html += QStringLiteral("<tr><td>环境温度</td><td>%1</td></tr>")
        .arg(formatDouble(boundary.ambientTemperature));
    html += QStringLiteral("</table>");

    html += QStringLiteral("<h3>仿真参数</h3><table border='1' cellspacing='0' cellpadding='6'>");
    html += QStringLiteral("<tr><th>参数</th><th>数值</th></tr>");
    html += QStringLiteral("<tr><td>仿真时间长度</td><td>%1</td></tr>")
        .arg(formatDouble(simulation.timeLength, 2));
    html += QStringLiteral("</table>");

    return html;
}

QString buildResultImageSectionHtml(
    const QString &projectPath,
    const ProjectInputHash::PostProcessManifest &manifest,
    ResultType type,
    const QString &sectionTitle)
{
    const int frameCount = manifest.odbFrames;
    if (frameCount <= 0) {
        return QString();
    }

    const int indices[] = {
        0,
        (frameCount - 1) / 2,
        frameCount - 1,
    };
    const QString labels[] = {
        QStringLiteral("初始时刻"),
        QStringLiteral("中间时刻"),
        QStringLiteral("最终时刻"),
    };

    QString html;
    html += QStringLiteral("<h3>%1</h3>").arg(htmlEscape(sectionTitle));

    for (int i = 0; i < 3; ++i) {
        const int frameIndex = indices[i];
        const QString pngPath =
            SimulationResultService::framePngPath(
                projectPath,
                type,
                frameIndex
            );

        html += QStringLiteral("<p><b>%1</b></p>").arg(labels[i]);

        if (manifest.frameTimes.size() == frameCount) {
            html += QStringLiteral(
                "<p>Frame %1 / %2<br/>仿真时间：%3 s</p>"
            ).arg(frameIndex + 1)
                .arg(frameCount)
                .arg(formatDouble(manifest.frameTimes.at(frameIndex), 2));
        } else {
            html += QStringLiteral(
                "<p>Frame %1 / %2</p>"
            ).arg(frameIndex + 1).arg(frameCount);
        }

        html += QStringLiteral(
            "<p><img src='%1' width='480'/></p>"
        ).arg(htmlEscape(QUrl::fromLocalFile(pngPath).toString()));
    }

    return html;
}

bool writeReportManifest(
    const QString &projectPath,
    const QString &postSha256,
    QString &errorMessage)
{
    QDir().mkpath(SimulationResultService::reportDirectoryPath(projectPath));

    const QJsonObject json = {
        {QStringLiteral("version"), 1},
        {QStringLiteral("postSha256"), postSha256},
        {
            QStringLiteral("generatedAt"),
            QDateTime::currentDateTime().toString(
                QStringLiteral("yyyy-MM-dd HH:mm:ss")
            )
        },
        {QStringLiteral("pdf"), QStringLiteral("simulation_report.pdf")},
    };

    QSaveFile file(SimulationResultService::reportManifestPath(projectPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errorMessage = QStringLiteral("无法写入报告清单。");
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        errorMessage = QStringLiteral("报告清单保存失败。");
        return false;
    }

    return true;
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

    ProjectConfig project;
    if (!ProjectManager::loadProject(projectPath, project)) {
        errorMessage = QStringLiteral("无法读取工程信息。");
        return false;
    }

    const QString jobName =
        ProjectInputHash::currentJobName(projectPath);
    const QString reportDir =
        SimulationResultService::reportDirectoryPath(projectPath);
    const QString pdfPath =
        SimulationResultService::reportPdfPath(projectPath);

    QDir().mkpath(reportDir);

    const QString generatedAt =
        QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss")
        );

    QString html;
    html += QStringLiteral("<html><head><meta charset='utf-8'/>");
    html += QStringLiteral(
        "<style>"
        "body { font-family: 'Microsoft YaHei', sans-serif; }"
        "h1,h2,h3 { color: #333333; }"
        "table { border-collapse: collapse; width: 100%; }"
        "th,td { border: 1px solid #cccccc; padding: 6px; }"
        "</style></head><body>"
    );

    html += QStringLiteral("<h1>PBX浇注固化仿真分析报告</h1>");
    html += QStringLiteral("<p>工程名称：%1</p>")
        .arg(htmlEscape(project.projectName));
    html += QStringLiteral("<p>Job名称：%1</p>").arg(htmlEscape(jobName));
    html += QStringLiteral("<p>报告生成时间：%1</p>").arg(generatedAt);
    html += QStringLiteral("<p>QT_PBX_204_ABAQUS</p>");

    html += buildParameterSectionHtml(projectPath);

    html += QStringLiteral("<h2>仿真结果</h2>");
    html += buildResultImageSectionHtml(
        projectPath,
        validation.manifest,
        ResultType::Cure,
        QStringLiteral("固化度结果")
    );
    html += buildResultImageSectionHtml(
        projectPath,
        validation.manifest,
        ResultType::Temperature,
        QStringLiteral("温度场结果")
    );
    html += buildResultImageSectionHtml(
        projectPath,
        validation.manifest,
        ResultType::Stress,
        QStringLiteral("Mises应力结果")
    );

    html += QStringLiteral("<h2>说明</h2>");
    html += QStringLiteral(
        "<p>本报告记录当前工程输入参数及 Abaqus 固化仿真后处理结果。</p>"
        "<p>结果包含：固化度场、温度场、Mises应力场。</p>"
        "<p>详细动态过程请查看工程 results 目录中的对应视频。</p>"
    );
    html += QStringLiteral("</body></html>");

    QTextDocument document;
    document.setHtml(html);

    QPdfWriter pdfWriter(pdfPath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);

    document.print(&pdfWriter);

    if (!QFile::exists(pdfPath) || QFileInfo(pdfPath).size() <= 0) {
        errorMessage = QStringLiteral("PDF 报告生成失败。");
        return false;
    }

    if (!writeReportManifest(
            projectPath,
            validation.manifest.postSha256,
            errorMessage)) {
        return false;
    }

    return true;
}
