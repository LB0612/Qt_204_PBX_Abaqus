#ifndef KNEADINGSIMULATIONWIDGET_H
#define KNEADINGSIMULATIONWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class KneadingSimulationWidget : public BaseParamWidget {
    Q_OBJECT
public:
    explicit KneadingSimulationWidget(QWidget *parent = nullptr);
    void setData(const NieheSimulationData &data);
    void getData(NieheSimulationData &data) const;
    void setSaveCallback(std::function<void(const NieheSimulationData&)> cb);

private:
    void setupUi();

    // 【统一风格】分行声明并初始化
    QLineEdit *maxTime = nullptr;
    QLineEdit *initTimeStep = nullptr;
    QLineEdit *minTimeStep = nullptr;
    QLineEdit *maxTimeStep = nullptr;
    QLineEdit *tolerance = nullptr;
    QLineEdit *maxSuccessSteps = nullptr;

    std::function<void(const NieheSimulationData&)> m_saveCallback;
};

#endif
