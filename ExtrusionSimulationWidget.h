#ifndef EXTRUSIONSIMULATIONWIDGET_H
#define EXTRUSIONSIMULATIONWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class ExtrusionSimulationWidget : public BaseParamWidget {
    Q_OBJECT
public:
    explicit ExtrusionSimulationWidget(QWidget *parent = nullptr);
    void setData(const ExtrusionSimulationData &data);
    void getData(ExtrusionSimulationData &data) const;
    void setSaveCallback(std::function<void(const ExtrusionSimulationData&)> cb);

private:
    void setupUi();

    // 【统一风格】
    QLineEdit *maxTime = nullptr;
    QLineEdit *initTimeStep = nullptr;
    QLineEdit *minTimeStep = nullptr;
    QLineEdit *maxTimeStep = nullptr;
    QLineEdit *tolerance = nullptr;
    QLineEdit *maxSuccessSteps = nullptr;

    std::function<void(const ExtrusionSimulationData&)> m_saveCallback;
};

#endif
