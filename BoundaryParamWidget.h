#ifndef BOUNDARYPARAMWIDGET_H
#define BOUNDARYPARAMWIDGET_H

#include "BaseParamWidget.h"
#include "BoundaryConfig.h"

class BoundaryParamWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit BoundaryParamWidget(QWidget *parent = nullptr);

    void setConfig(const BoundaryConfig &config);
    BoundaryConfig getConfig() const;

signals:
    void saveRequested();

private:
    void setupUi();

    QLineEdit *ambientTemperatureEdit;
};

#endif
