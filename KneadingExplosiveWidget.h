#ifndef KNEADINGEXPLOSIVEWIDGET_H
#define KNEADINGEXPLOSIVEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QVBoxLayout> // 【修复】添加布局头文件
#include <QGroupBox>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class KneadingExplosiveWidget : public BaseParamWidget
{
    Q_OBJECT
public:
    explicit KneadingExplosiveWidget(QWidget *parent = nullptr);
    void setData(const NieheExplosiveData &data);
    void getData(NieheExplosiveData &data) const;
    void setSaveCallback(std::function<void(const NieheExplosiveData&)> callback);

private:
    void setupUi(); // 新增结构化函数

    // 炸药参数 - 安全初始化
    QLineEdit *density = nullptr;
    QLineEdit *specificHeat = nullptr;
    QLineEdit *conductivity = nullptr;
    QLineEdit *initialTemp = nullptr;
    QLineEdit *powerLawFac = nullptr;
    QLineEdit *powerLawTnat = nullptr;
    QLineEdit *powerLawExpo = nullptr;

    std::function<void(const NieheExplosiveData&)> m_saveCallback;
};

#endif // KNEADINGEXPLOSIVEWIDGET_H