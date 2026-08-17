#ifndef EXTRUSIONSCREWWIDGET_H
#define EXTRUSIONSCREWWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class ExtrusionScrewWidget : public BaseParamWidget {
    Q_OBJECT
public:
    explicit ExtrusionScrewWidget(QWidget *parent = nullptr);
    void setData(const ExtrusionScrewData &data);
    void getData(ExtrusionScrewData &data) const;
    void setSaveCallback(std::function<void(const ExtrusionScrewData&)> callback);

private:
    void setupUi();

    // 【变量名与 ProjectManager.h 结构体完全对应】
    QLineEdit *solidRotationPointX = nullptr;
    QLineEdit *solidRotationPointY = nullptr;
    QLineEdit *solidRotationPointZ = nullptr;
    QLineEdit *solidRotationAxisX = nullptr;
    QLineEdit *solidRotationAxisY = nullptr;
    QLineEdit *solidRotationAxisZ = nullptr;
    QLineEdit *solidBladeSpeed = nullptr;
    QLineEdit *solidBladeDensity = nullptr;
    QLineEdit *solidBladeThermalConductivity = nullptr;
    QLineEdit *solidBladeHeatTransfer = nullptr;

    std::function<void(const ExtrusionScrewData&)> m_saveCallback;
};

#endif // EXTRUSIONSCREWWIDGET_H