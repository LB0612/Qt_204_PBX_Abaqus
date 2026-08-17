#include "KneadingBoundaryWidget.h"

KneadingBoundaryWidget::KneadingBoundaryWidget(QWidget *parent) : BaseParamWidget(parent) {
    setupUi();
    applyCommonStyles();
}

void KneadingBoundaryWidget::setupUi() {
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("边界条件 (捏合)");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 1. 初始化
    wallUpTemp = createSciEdit();
    wallTemp = createSciEdit();
    wallDownTemp = createSciEdit();
    rotationPoint1X = createSciEdit();
    rotationPoint1Y = createSciEdit();
    rotationPoint1Z = createSciEdit();
    rotationPoint2X = createSciEdit();
    rotationPoint2Y = createSciEdit();
    rotationPoint2Z = createSciEdit();
    wallRotationSpeed = createSciEdit();

    // 2. 布局
    addParamRow(scrollLayout, "上壁面温度", wallUpTemp, "K");
    addParamRow(scrollLayout, "壁面温度", wallTemp, "K");
    addParamRow(scrollLayout, "下壁面温度", wallDownTemp, "K");
    addParamRow(scrollLayout, "旋转点1x", rotationPoint1X, "mm");
    addParamRow(scrollLayout, "旋转点1y", rotationPoint1Y, "mm");
    addParamRow(scrollLayout, "旋转点1z", rotationPoint1Z, "mm");
    addParamRow(scrollLayout, "旋转点2x", rotationPoint2X, "mm");
    addParamRow(scrollLayout, "旋转点2y", rotationPoint2Y, "mm");
    addParamRow(scrollLayout, "旋转点2z", rotationPoint2Z, "mm");
    addParamRow(scrollLayout, "壁面自转速度", wallRotationSpeed, "m/s");
    
    addSaveButton(scrollLayout, "保存边界条件", [this](){
        if(m_saveCallback) {
            NieheBoundaryData d;
            getData(d);
            m_saveCallback(d);
        }
    });

    scrollLayout->addStretch(); // 底部顶起

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void KneadingBoundaryWidget::setData(const NieheBoundaryData &d) {
    if(!wallUpTemp) return;
    wallUpTemp->setText(d.wallUpTemp);
    wallTemp->setText(d.wallTemp);
    wallDownTemp->setText(d.wallDownTemp);
    rotationPoint1X->setText(d.rotationPoint1X);
    rotationPoint1Y->setText(d.rotationPoint1Y);
    rotationPoint1Z->setText(d.rotationPoint1Z);
    rotationPoint2X->setText(d.rotationPoint2X);
    rotationPoint2Y->setText(d.rotationPoint2Y);
    rotationPoint2Z->setText(d.rotationPoint2Z);
    wallRotationSpeed->setText(d.wallRotationSpeed);
}

void KneadingBoundaryWidget::getData(NieheBoundaryData &d) const {
    if(!wallUpTemp) return;
    d.wallUpTemp = wallUpTemp->text();
    d.wallTemp = wallTemp->text();
    d.wallDownTemp = wallDownTemp->text();
    d.rotationPoint1X = rotationPoint1X->text();
    d.rotationPoint1Y = rotationPoint1Y->text();
    d.rotationPoint1Z = rotationPoint1Z->text();
    d.rotationPoint2X = rotationPoint2X->text();
    d.rotationPoint2Y = rotationPoint2Y->text();
    d.rotationPoint2Z = rotationPoint2Z->text();
    d.wallRotationSpeed = wallRotationSpeed->text();
}

void KneadingBoundaryWidget::setSaveCallback(std::function<void(const NieheBoundaryData&)> cb) {
    m_saveCallback = cb;
}
