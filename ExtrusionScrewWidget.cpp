#include "ExtrusionScrewWidget.h"
#include <cmath>

ExtrusionScrewWidget::ExtrusionScrewWidget(QWidget *parent) : BaseParamWidget(parent) {
    setupUi();
    applyCommonStyles();
}

void ExtrusionScrewWidget::setupUi() {
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("螺杆结构与运动参数 (挤压工艺)");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 初始化变量，确保与头文件一致
    solidRotationPointX = createSciEdit();
    solidRotationPointY = createSciEdit();
    solidRotationPointZ = createSciEdit();
    solidRotationAxisX = createSciEdit();
    solidRotationAxisY = createSciEdit();
    solidRotationAxisZ = createSciEdit();
    solidBladeSpeed = createSciEdit();
    solidBladeDensity = createSciEdit();
    solidBladeThermalConductivity = createSciEdit();
    solidBladeHeatTransfer = createSciEdit();

    // 添加到布局
    addParamRow(scrollLayout, "旋转点坐标x", solidRotationPointX, "mm");
    addParamRow(scrollLayout, "旋转点坐标y", solidRotationPointY, "mm");
    addParamRow(scrollLayout, "旋转点坐标z", solidRotationPointZ, "mm");
    addParamRow(scrollLayout, "旋转轴方向x", solidRotationAxisX, "-");
    addParamRow(scrollLayout, "旋转轴方向y", solidRotationAxisY, "-");
    addParamRow(scrollLayout, "旋转轴方向z", solidRotationAxisZ, "-");
    addParamRow(scrollLayout, "转速", solidBladeSpeed, "r/min");
    addParamRow(scrollLayout, "密度", solidBladeDensity, "kg/m³");
    addParamRow(scrollLayout, "热导率", solidBladeThermalConductivity, "W/(m·K)");
    addParamRow(scrollLayout, "比热", solidBladeHeatTransfer, "J/(kg·K)");

    addSaveButton(scrollLayout, "保存螺杆参数", [this](){
        if(m_saveCallback) {
            ExtrusionScrewData d;
            getData(d);
            m_saveCallback(d);
        }
    });
    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void ExtrusionScrewWidget::setSaveCallback(std::function<void(const ExtrusionScrewData&)> cb) {
    m_saveCallback = cb;
}

void ExtrusionScrewWidget::getData(ExtrusionScrewData &d) const {
    // 访问结构体中正确的成员变量
    d.solidRotationPointX = solidRotationPointX->text();
    d.solidRotationPointY = solidRotationPointY->text();
    d.solidRotationPointZ = solidRotationPointZ->text();
    d.solidRotationAxisX = solidRotationAxisX->text();
    d.solidRotationAxisY = solidRotationAxisY->text();
    d.solidRotationAxisZ = solidRotationAxisZ->text();
    
    // 将 RPM 转换为 rad/s (弧度每秒)
    bool ok;
    double rpm = solidBladeSpeed->text().toDouble(&ok);
    if (ok) {
        double rads = rpm * 2 * M_PI / 60.0;
        d.solidBladeSpeed = QString::asprintf("%1.7E", rads);
    } else {
        d.solidBladeSpeed = solidBladeSpeed->text();
    }
    
    d.solidBladeDensity = solidBladeDensity->text();
    d.solidBladeThermalConductivity = solidBladeThermalConductivity->text();
    d.solidBladeHeatTransfer = solidBladeHeatTransfer->text();
}

void ExtrusionScrewWidget::setData(const ExtrusionScrewData &d) {
    // 访问结构体中正确的成员变量
    solidRotationPointX->setText(d.solidRotationPointX);
    solidRotationPointY->setText(d.solidRotationPointY);
    solidRotationPointZ->setText(d.solidRotationPointZ);
    solidRotationAxisX->setText(d.solidRotationAxisX);
    solidRotationAxisY->setText(d.solidRotationAxisY);
    solidRotationAxisZ->setText(d.solidRotationAxisZ);
    solidBladeSpeed->setText(d.solidBladeSpeed);
    solidBladeDensity->setText(d.solidBladeDensity);
    solidBladeThermalConductivity->setText(d.solidBladeThermalConductivity);
    solidBladeHeatTransfer->setText(d.solidBladeHeatTransfer);
}