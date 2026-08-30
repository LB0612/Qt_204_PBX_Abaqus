#ifndef SIMULATIONREPORTPDFWRITER_H
#define SIMULATIONREPORTPDFWRITER_H

#include "SimulationReportModel.h"

#include <QString>

class SimulationReportPdfWriter
{
public:
    static bool write(
        const SimulationReportModel &model,
        const QString &outputPath,
        QString &errorMessage
    );
};

#endif
