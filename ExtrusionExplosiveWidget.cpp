#include "ExtrusionExplosiveWidget.h"

ExtrusionExplosiveWidget::ExtrusionExplosiveWidget(QWidget *parent) : BaseParamWidget(parent) {
    setupUi();
    applyCommonStyles();
}

void ExtrusionExplosiveWidget::setupUi() {
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("炸药材料参数 (挤压工艺)");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 1. 初始化控件 (变量名已与结构体统一)
    density = createSciEdit();
    specificHeat = createSciEdit();
    conductivity = createSciEdit();
    initialTemp = createSciEdit();
    powerLawFac = createSciEdit();
    powerLawTnat = createSciEdit();
    powerLawExpo = createSciEdit();

    // 2. 布局
    addParamRow(scrollLayout, "炸药流体密度:", density, "kg/m³"); // 建议补充单位
    addParamRow(scrollLayout, "比热:", specificHeat, "J/(kg·K)");
    addParamRow(scrollLayout, "热导率:", conductivity, "W/(m·K)");
    addParamRow(scrollLayout, "炸药初始温度:", initialTemp, "K");
    addParamRow(scrollLayout, "幂律定律fac:", powerLawFac, "Pa·sⁿ");
    addParamRow(scrollLayout, "幂律定律tnat:", powerLawTnat, "-");
    addParamRow(scrollLayout, "幂律定律expo:", powerLawExpo, "-");
    
    addSaveButton(scrollLayout, "保存炸药参数", [this](){
        if(m_saveCallback) {
            ExtrusionExplosiveData d;
            getData(d);
            m_saveCallback(d);
        }
    });
    
    scrollLayout->addStretch();
    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void ExtrusionExplosiveWidget::setData(const ExtrusionExplosiveData &d) {
    // 统一后，代码极度整洁，无需脑补映射
    density->setText(d.density);
    specificHeat->setText(d.specificHeat);
    conductivity->setText(d.conductivity);
    initialTemp->setText(d.initialTemp);
    powerLawFac->setText(d.powerLawFac);
    powerLawTnat->setText(d.powerLawTnat);
    powerLawExpo->setText(d.powerLawExpo);
}

void ExtrusionExplosiveWidget::getData(ExtrusionExplosiveData &d) const {
    d.density = density->text();
    d.specificHeat = specificHeat->text();
    d.conductivity = conductivity->text();
    d.initialTemp = initialTemp->text();
    d.powerLawFac = powerLawFac->text();
    d.powerLawTnat = powerLawTnat->text();
    d.powerLawExpo = powerLawExpo->text();
}

void ExtrusionExplosiveWidget::setSaveCallback(std::function<void(const ExtrusionExplosiveData&)> callback) {
    m_saveCallback = callback;
}
