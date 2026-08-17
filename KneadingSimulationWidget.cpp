#include "KneadingSimulationWidget.h"
#include <QGroupBox>
#include <QScrollArea>

KneadingSimulationWidget::KneadingSimulationWidget(QWidget *parent) : BaseParamWidget(parent) {
    setupUi();
    applyCommonStyles();
}

void KneadingSimulationWidget::setupUi() {
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("仿真控制参数 (捏合工艺)");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 1. 初始化
    maxTime = createSciEdit();
    initTimeStep = createSciEdit();
    minTimeStep = createSciEdit();
    maxTimeStep = createSciEdit();
    tolerance = createSciEdit();
    maxSuccessSteps = createSciEdit();

    // 2. 布局
    addParamRow(scrollLayout, "模拟的最大时间限制", maxTime, "s");
    addParamRow(scrollLayout, "时间步长初始值", initTimeStep, "s");
    addParamRow(scrollLayout, "时间步长最小值", minTimeStep, "s");
    addParamRow(scrollLayout, "时间步长最大值", maxTimeStep, "s");
    addParamRow(scrollLayout, "时间推进容差值", tolerance, "-");
    addParamRow(scrollLayout, "成功步数最大值", maxSuccessSteps, "-");

    addSaveButton(scrollLayout, "保存仿真设置", [this](){ 
        if(m_saveCallback) { 
            NieheSimulationData d; 
            getData(d); 
            m_saveCallback(d); 
        } 
    });
    scrollLayout->addStretch();
    scrollArea->setWidget(content); 
    mainLayout->addWidget(scrollArea);
}

void KneadingSimulationWidget::setData(const NieheSimulationData &d) {
    maxTime->setText(d.maxTime);
    initTimeStep->setText(d.initTimeStep);
    minTimeStep->setText(d.minTimeStep);
    maxTimeStep->setText(d.maxTimeStep);
    tolerance->setText(d.tolerance);
    maxSuccessSteps->setText(d.maxSuccessSteps);
}

void KneadingSimulationWidget::getData(NieheSimulationData &d) const {
    d.maxTime = maxTime->text();
    d.initTimeStep = initTimeStep->text();
    d.minTimeStep = minTimeStep->text();
    d.maxTimeStep = maxTimeStep->text();
    d.tolerance = tolerance->text();
    d.maxSuccessSteps = maxSuccessSteps->text();
}

void KneadingSimulationWidget::setSaveCallback(std::function<void(const NieheSimulationData&)> cb) {
    m_saveCallback = cb;
}
