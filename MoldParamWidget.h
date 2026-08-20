#ifndef MOLDPARAMWIDGET_H
#define MOLDPARAMWIDGET_H

#include "BaseParamWidget.h"
#include "MoldConfig.h"

class MoldParamWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit MoldParamWidget(QWidget *parent = nullptr);

    void setConfig(const MoldConfig &config);
    MoldConfig getConfig() const;

signals:
    void saveRequested();

private:
    void setupUi();

    QLineEdit *densityEdit;
    QLineEdit *elasticModulusEdit;
    QLineEdit *poissonRatioEdit;
    QLineEdit *thermalConductivityEdit;
    QLineEdit *specificHeatEdit;
};

#endif
