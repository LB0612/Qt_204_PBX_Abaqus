#ifndef KNEADINGBOUNDARYWIDGET_H
#define KNEADINGBOUNDARYWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <functional>
#include "ProjectManager.h"
#include "BaseParamWidget.h"
class KneadingBoundaryWidget : public BaseParamWidget
{
    Q_OBJECT
public:
    explicit KneadingBoundaryWidget(QWidget *parent = nullptr);
    void setData(const NieheBoundaryData &data);
    void getData(NieheBoundaryData &data) const;
    void setSaveCallback(std::function<void(const NieheBoundaryData&)> callback);

private:
    void setupUi();

    // 统一初始化
    QLineEdit *wallUpTemp = nullptr;
    QLineEdit *wallTemp = nullptr;
    QLineEdit *wallDownTemp = nullptr;
    QLineEdit *rotationPoint1X = nullptr;
    QLineEdit *rotationPoint1Y = nullptr;
    QLineEdit *rotationPoint1Z = nullptr;
    QLineEdit *rotationPoint2X = nullptr;
    QLineEdit *rotationPoint2Y = nullptr;
    QLineEdit *rotationPoint2Z = nullptr;
    QLineEdit *wallRotationSpeed = nullptr;

    std::function<void(const NieheBoundaryData&)> m_saveCallback;
};

#endif // KNEADINGBOUNDARYWIDGET_H