#ifndef SIMULATIONREPORTMODEL_H
#define SIMULATIONREPORTMODEL_H

#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

struct SimulationReportRow
{
    QString name;
    QString value;
    QString unit;
};

struct SimulationReportTable
{
    QString title;
    QVector<SimulationReportRow> rows;
};

struct SimulationReportFigure
{
    QString label;
    QString frameText;
    QString timeText;
    QString imagePath;
    QSize imagePixelSize;
};

struct SimulationReportResultSection
{
    QString title;
    QVector<SimulationReportFigure> figures;
};

struct SimulationReportModel
{
    int reportFormatVersion = 4;

    QString reportTitle;
    QString productName;
    QString appVersion;

    QString projectName;
    QString jobName;

    QString generatedAt;
    QString t2CompletedAt;
    QString postSha256;

    QVector<SimulationReportRow> overviewRows;
    QVector<SimulationReportTable> parameterTables;
    QVector<SimulationReportResultSection> resultSections;

    QStringList notes;
    QVector<SimulationReportRow> traceRows;
};

#endif
