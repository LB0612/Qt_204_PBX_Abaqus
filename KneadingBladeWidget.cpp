#include "KneadingBladeWidget.h"
#include <cmath> 

KneadingBladeWidget::KneadingBladeWidget(QWidget *parent) : BaseParamWidget(parent) { 
    setupUi(); 
    applyCommonStyles(); 
} 

void KneadingBladeWidget::setupUi() { 
    QVBoxLayout *mainLayout = createMainLayout(this); 
    setupHeader("桨叶结构与运动参数 (捏合工艺)"); 

    QScrollArea *scrollArea = createScrollArea(this); 
    QWidget *content = new QWidget(); 
    content->setObjectName("ScrollContent"); 
    QVBoxLayout *scrollLayout = createScrollContentLayout(content); 

    // 初始化所有输入框 (与结构体同名) 
    solidRotationPointX = createSciEdit(); 
    solidRotationPointY = createSciEdit(); 
    solidRotationPointZ = createSciEdit(); 
    solidRotationAxisX = createSciEdit(); 
    solidRotationAxisY = createSciEdit(); 
    solidRotationAxisZ = createSciEdit(); 
    solidBladeDensity = createSciEdit(); 
    solidBladeThermalConductivity = createSciEdit(); 
    solidBladeHeatTransfer = createSciEdit(); 
    solidBladeSpeed = createSciEdit(); 

    hollowRotationPointX = createSciEdit(); 
    hollowRotationPointY = createSciEdit(); 
    hollowRotationPointZ = createSciEdit(); 
    hollowRotationAxisX = createSciEdit(); 
    hollowRotationAxisY = createSciEdit(); 
    hollowRotationAxisZ = createSciEdit(); 
    hollowBladeDensity = createSciEdit(); 
    hollowBladeThermalConductivity = createSciEdit(); 
    hollowBladeHeatTransfer = createSciEdit(); 
    hollowBladeSpeed = createSciEdit(); 

    // 布局 
    addSectionTitle(scrollLayout, "实心桨参数"); 
    addParamRow(scrollLayout, "旋转点坐标x", solidRotationPointX, ""); 
    addParamRow(scrollLayout, "旋转点坐标y", solidRotationPointY, ""); 
    addParamRow(scrollLayout, "旋转点坐标z", solidRotationPointZ, ""); 
    addParamRow(scrollLayout, "旋转轴方向x", solidRotationAxisX, ""); 
    addParamRow(scrollLayout, "旋转轴方向y", solidRotationAxisY, ""); 
    addParamRow(scrollLayout, "旋转轴方向z", solidRotationAxisZ, ""); 
    addParamRow(scrollLayout, "密度", solidBladeDensity, "kg/m³"); 
    addParamRow(scrollLayout, "热导率", solidBladeThermalConductivity, "W/(m·K)"); 
    addParamRow(scrollLayout, "比热", solidBladeHeatTransfer, "J/(kg·K)"); 
    addParamRow(scrollLayout, "转速", solidBladeSpeed, "r/min"); 

    addSectionTitle(scrollLayout, "空心桨参数"); 
    addParamRow(scrollLayout, "旋转点坐标x", hollowRotationPointX, "mm"); 
    addParamRow(scrollLayout, "旋转点坐标y", hollowRotationPointY, "mm"); 
    addParamRow(scrollLayout, "旋转点坐标z", hollowRotationPointZ, "mm"); 
    addParamRow(scrollLayout, "旋转轴方向x", hollowRotationAxisX, "-"); 
    addParamRow(scrollLayout, "旋转轴方向y", hollowRotationAxisY, "-"); 
    addParamRow(scrollLayout, "旋转轴方向z", hollowRotationAxisZ, "-"); 
    addParamRow(scrollLayout, "密度", hollowBladeDensity, "kg/m³"); 
    addParamRow(scrollLayout, "热导率", hollowBladeThermalConductivity, "W/(m·K)"); 
    addParamRow(scrollLayout, "比热", hollowBladeHeatTransfer, "J/(kg·K)"); 
    addParamRow(scrollLayout, "转速", hollowBladeSpeed, "r/min"); 

    addSaveButton(scrollLayout, "保存桨叶参数", [this](){
        if(m_saveCallback) { 
            NieheParameters d; 
            getData(d); 
            m_saveCallback(d); 
        } 
    }); 
    
    scrollLayout->addStretch(); 
    scrollArea->setWidget(content); 
    mainLayout->addWidget(scrollArea); 
} 

void KneadingBladeWidget::setSaveCallback(std::function<void(const NieheParameters&)> cb)
{
    m_saveCallback = cb;
} 

void KneadingBladeWidget::getData(NieheParameters &d) const
{
    d.solidRotationPointX = solidRotationPointX->text();
    d.solidRotationPointY = solidRotationPointY->text();
    d.solidRotationPointZ = solidRotationPointZ->text();
    d.solidRotationAxisX = solidRotationAxisX->text();
    d.solidRotationAxisY = solidRotationAxisY->text();
    d.solidRotationAxisZ = solidRotationAxisZ->text();
    d.solidBladeDensity = solidBladeDensity->text();
    d.solidBladeThermalConductivity = solidBladeThermalConductivity->text();
    d.solidBladeSpecificHeat = solidBladeHeatTransfer->text();
    d.solidBladeSpeed = solidBladeSpeed->text();

    d.hollowRotationPointX = hollowRotationPointX->text();
    d.hollowRotationPointY = hollowRotationPointY->text();
    d.hollowRotationPointZ = hollowRotationPointZ->text();
    d.hollowRotationAxisX = hollowRotationAxisX->text();
    d.hollowRotationAxisY = hollowRotationAxisY->text();
    d.hollowRotationAxisZ = hollowRotationAxisZ->text();
    d.hollowBladeDensity = hollowBladeDensity->text();
    d.hollowBladeThermalConductivity = hollowBladeThermalConductivity->text();
    d.hollowBladeSpecificHeat = hollowBladeHeatTransfer->text();
    d.hollowBladeSpeed = hollowBladeSpeed->text();
}

void KneadingBladeWidget::setData(const NieheParameters &d)
{
    solidRotationPointX->setText(d.solidRotationPointX);
    solidRotationPointY->setText(d.solidRotationPointY);
    solidRotationPointZ->setText(d.solidRotationPointZ);
    solidRotationAxisX->setText(d.solidRotationAxisX);
    solidRotationAxisY->setText(d.solidRotationAxisY);
    solidRotationAxisZ->setText(d.solidRotationAxisZ);
    solidBladeDensity->setText(d.solidBladeDensity);
    solidBladeThermalConductivity->setText(d.solidBladeThermalConductivity);
    solidBladeHeatTransfer->setText(d.solidBladeSpecificHeat);
    solidBladeSpeed->setText(d.solidBladeSpeed);

    hollowRotationPointX->setText(d.hollowRotationPointX);
    hollowRotationPointY->setText(d.hollowRotationPointY);
    hollowRotationPointZ->setText(d.hollowRotationPointZ);
    hollowRotationAxisX->setText(d.hollowRotationAxisX);
    hollowRotationAxisY->setText(d.hollowRotationAxisY);
    hollowRotationAxisZ->setText(d.hollowRotationAxisZ);
    hollowBladeDensity->setText(d.hollowBladeDensity);
    hollowBladeThermalConductivity->setText(d.hollowBladeThermalConductivity);
    hollowBladeHeatTransfer->setText(d.hollowBladeSpecificHeat);
    hollowBladeSpeed->setText(d.hollowBladeSpeed);
}