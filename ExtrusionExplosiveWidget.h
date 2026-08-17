#ifndef EXTRUSIONEXPLOSIVEWIDGET_H
#define EXTRUSIONEXPLOSIVEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class ExtrusionExplosiveWidget : public BaseParamWidget
{
    Q_OBJECT
public:
    explicit ExtrusionExplosiveWidget(QWidget *parent = nullptr);
    void setData(const ExtrusionExplosiveData &data);
    void getData(ExtrusionExplosiveData &data) const;
    void setSaveCallback(std::function<void(const ExtrusionExplosiveData&)> callback);

private:
    void setupUi();

    // 【变量名已修正：与 ProjectManager.h 结构体完全对应】
    QLineEdit *density = nullptr;
    QLineEdit *specificHeat = nullptr;
    QLineEdit *conductivity = nullptr;
    QLineEdit *initialTemp = nullptr;
    QLineEdit *powerLawFac = nullptr;
    QLineEdit *powerLawTnat = nullptr;
    QLineEdit *powerLawExpo = nullptr;

    std::function<void(const ExtrusionExplosiveData&)> m_saveCallback;
};

#endif // EXTRUSIONEXPLOSIVEWIDGET_H