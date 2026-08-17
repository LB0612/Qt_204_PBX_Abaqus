#include "KneadingExplosiveWidget.h"
#include <QVBoxLayout> // 【修复】必须包含，否则 createMainLayout 返回的类型无法使用
#include <QScrollArea> // 【修复】必须包含，否则 QScrollArea 无法使用

KneadingExplosiveWidget::KneadingExplosiveWidget(QWidget *parent) : BaseParamWidget(parent) {
    setupUi();
    applyCommonStyles(); // 确保样式生效
}

void KneadingExplosiveWidget::setupUi() {
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("炸药材料参数 (捏合工艺)");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 1. 初始化
    density = createSciEdit();
    specificHeat = createSciEdit();
    conductivity = createSciEdit();
    initialTemp = createSciEdit();
    powerLawFac = createSciEdit();
    powerLawTnat = createSciEdit();
    powerLawExpo = createSciEdit();

    // 2. 布局 - 【优化：补全单位，与 Extrusion 版本保持一致】
    addParamRow(scrollLayout, "炸药流体密度:", density, "kg/m³");
    addParamRow(scrollLayout, "比热:", specificHeat, "J/(kg·K)");
    addParamRow(scrollLayout, "热导率:", conductivity, "W/(m·K)");
    addParamRow(scrollLayout, "炸药初始温度:", initialTemp, "K");
    addParamRow(scrollLayout, "幂律定律fac:", powerLawFac, "Pa·sⁿ");
    addParamRow(scrollLayout, "幂律定律tnat:", powerLawTnat, "-");
    addParamRow(scrollLayout, "幂律定律expo:", powerLawExpo, "-");

    // 【关键修改】先加按钮，再加 Stretch
    addSaveButton(scrollLayout, "保存炸药参数", [this](){
        if(m_saveCallback) {
            NieheExplosiveData d;
            getData(d);
            m_saveCallback(d);
        }
    });

    scrollLayout->addStretch(); // 放在最后！

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void KneadingExplosiveWidget::setSaveCallback(std::function<void(const NieheExplosiveData&)> callback) {
    m_saveCallback = callback;
}

void KneadingExplosiveWidget::setData(const NieheExplosiveData &d) {
    if(!density) return;
    density->setText(d.density);
    specificHeat->setText(d.specificHeat);
    conductivity->setText(d.conductivity);
    initialTemp->setText(d.initialTemp);
    powerLawFac->setText(d.powerLawFac);
    powerLawTnat->setText(d.powerLawTnat);
    powerLawExpo->setText(d.powerLawExpo);
}

void KneadingExplosiveWidget::getData(NieheExplosiveData &d) const {
    if(!density) return;
    d.density = density->text();
    d.specificHeat = specificHeat->text();
    d.conductivity = conductivity->text();
    d.initialTemp = initialTemp->text();
    d.powerLawFac = powerLawFac->text();
    d.powerLawTnat = powerLawTnat->text();
    d.powerLawExpo = powerLawExpo->text();
}
