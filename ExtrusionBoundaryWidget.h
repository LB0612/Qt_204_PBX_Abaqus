#ifndef EXTRUSIONBOUNDARYWIDGET_H
#define EXTRUSIONBOUNDARYWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"

class ExtrusionBoundaryWidget : public BaseParamWidget {
    Q_OBJECT
public:
    explicit ExtrusionBoundaryWidget(QWidget *parent = nullptr);
    void setData(const ExtrusionBoundaryData &data);
    void getData(ExtrusionBoundaryData &data) const;
    void setSaveCallback(std::function<void(const ExtrusionBoundaryData&)> callback);

private:
    void setupUi();
    
    // 【安全初始化】+ 单行声明
    QLineEdit *wallUpTemp = nullptr;
    QLineEdit *wallTemp = nullptr;
    QLineEdit *wallDownTemp = nullptr;
    QLineEdit *rotationPoint1X = nullptr;
    QLineEdit *rotationPoint1Y = nullptr;
    QLineEdit *rotationPoint1Z = nullptr;
    
    std::function<void(const ExtrusionBoundaryData&)> m_saveCallback;
};

#endif // EXTRUSIONBOUNDARYWIDGET_H