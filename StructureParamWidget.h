#ifndef STRUCTUREPARAMWIDGET_H
#define STRUCTUREPARAMWIDGET_H

#include "BaseParamWidget.h"
#include "StructureConfig.h"

class StructureParamWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit StructureParamWidget(QWidget *parent = nullptr);

    void setConfig(const StructureConfig &config);
    StructureConfig getConfig() const;

signals:
    void saveRequested();

private:
    void setupUi();

    QLineEdit *radiusEdit;
    QLineEdit *heightEdit;
    QLineEdit *shellThicknessEdit;
};

#endif
