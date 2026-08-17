/**
 * @file KneadingBladeWidget.h
 * @brief 桨叶结构与运动参数配置界面类的头文件
 */
#ifndef KNEADINGBLADEWIDGET_H
#define KNEADINGBLADEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class KneadingBladeWidget : public BaseParamWidget {
    Q_OBJECT
public:
    explicit KneadingBladeWidget(QWidget *parent = nullptr);
    void setData(const NieheParameters &data);
    void getData(NieheParameters &data) const;
    void setSaveCallback(std::function<void(const NieheParameters&)> callback);

private:
    void setupUi();

    // 实心桨参数
    QLineEdit *solidRotationPointX = nullptr;
    QLineEdit *solidRotationPointY = nullptr;
    QLineEdit *solidRotationPointZ = nullptr;
    QLineEdit *solidRotationAxisX = nullptr;
    QLineEdit *solidRotationAxisY = nullptr;
    QLineEdit *solidRotationAxisZ = nullptr;
    QLineEdit *solidBladeDensity = nullptr;
    QLineEdit *solidBladeThermalConductivity = nullptr;
    QLineEdit *solidBladeHeatTransfer = nullptr;
    QLineEdit *solidBladeSpeed = nullptr;

    // 空心桨参数
    QLineEdit *hollowRotationPointX = nullptr;
    QLineEdit *hollowRotationPointY = nullptr;
    QLineEdit *hollowRotationPointZ = nullptr;
    QLineEdit *hollowRotationAxisX = nullptr;
    QLineEdit *hollowRotationAxisY = nullptr;
    QLineEdit *hollowRotationAxisZ = nullptr;
    QLineEdit *hollowBladeDensity = nullptr;
    QLineEdit *hollowBladeThermalConductivity = nullptr;
    QLineEdit *hollowBladeHeatTransfer = nullptr;
    QLineEdit *hollowBladeSpeed = nullptr;

    std::function<void(const NieheParameters&)> m_saveCallback;
};

#endif // KNEADINGBLADEWIDGET_H