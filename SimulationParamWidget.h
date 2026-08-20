#ifndef SIMULATIONPARAMWIDGET_H
#define SIMULATIONPARAMWIDGET_H

#include "BaseParamWidget.h"
#include "SimulationConfig.h"

class SimulationParamWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit SimulationParamWidget(QWidget *parent = nullptr);

    void setConfig(const SimulationConfig &config);
    SimulationConfig getConfig() const;

signals:
    void saveRequested();

private:
    void setupUi();

    QLineEdit *timeLengthEdit;
};

#endif
