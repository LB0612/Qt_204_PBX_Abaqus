#ifndef EXPLOSIVEPARAMWIDGET_H
#define EXPLOSIVEPARAMWIDGET_H

#include "BaseParamWidget.h"
#include "ExplosiveConfig.h"

class ExplosiveParamWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit ExplosiveParamWidget(QWidget *parent = nullptr);

    void setConfig(const ExplosiveConfig &config);
    ExplosiveConfig getConfig() const;

signals:
    void saveRequested();

private:
    void setupUi();

    QLineEdit *densityEdit;
    QLineEdit *initialElasticModulusEdit;
    QLineEdit *initialPoissonRatioEdit;
    QLineEdit *finalElasticModulusEdit;
    QLineEdit *finalPoissonRatioEdit;
    QLineEdit *thermalConductivityEdit;
    QLineEdit *yieldStressEdit;
    QLineEdit *specificHeatEdit;
    QLineEdit *expansionCoefficientEdit;
};

#endif
