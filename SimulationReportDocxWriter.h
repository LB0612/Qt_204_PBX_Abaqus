#ifndef SIMULATIONREPORTDOCXWRITER_H
#define SIMULATIONREPORTDOCXWRITER_H

#include "SimulationReportModel.h"

#include <QString>

class SimulationReportDocxWriter
{
public:
    static bool write(
        const SimulationReportModel &model,
        const QString &outputPath,
        QString &errorMessage,
        int bodyTotalPages
    );
};

#endif
