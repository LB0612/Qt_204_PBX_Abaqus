#ifndef SIMULATIONREPORTPDFWRITER_H
#define SIMULATIONREPORTPDFWRITER_H

#include "SimulationReportModel.h"

#include <QString>
#include <QVector>

class SimulationReportPdfWriter
{
public:
    static bool write(
        const SimulationReportModel &model,
        const QString &outputPath,
        QString &errorMessage,
        int *outBodyPageCount = nullptr,
        QVector<int> *outFigurePageStarts = nullptr
    );
};

#endif
