#include "ExtrusionBoundaryWidget.h"

ExtrusionBoundaryWidget::ExtrusionBoundaryWidget(QWidget *parent) : BaseParamWidget(parent) {
    setupUi();
    applyCommonStyles();
}

void ExtrusionBoundaryWidget::setupUi() {
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("挤压边界条件 (挤压工艺)");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 初始化控件
    wallUpTemp = createSciEdit();
    wallTemp = createSciEdit();
    wallDownTemp = createSciEdit();
    
    rotationPoint1X = createSciEdit();
    rotationPoint1Y = createSciEdit();
    rotationPoint1Z = createSciEdit();

    // -------------------------------------------------------------------------
    // 【UI 标签修正】:
    // 确保标签名称与 ProjectManager 中的 Polyflow 映射逻辑 (IN/OUT/WALL) 物理含义一致
    // -------------------------------------------------------------------------
    
    // 原 "上壁面温度" -> 映射到 IN (入口)
    addParamRow(scrollLayout, "入口段温度", wallUpTemp, "K");
    
    // 原 "壁面温度" -> 映射到 WALL (侧壁/机筒)
    addParamRow(scrollLayout, "机筒/侧壁温度", wallTemp, "K");
    
    // 原 "下壁面温度" -> 映射到 OUT (出口)
    addParamRow(scrollLayout, "出口段温度", wallDownTemp, "K");

    // 原 "旋转点1" -> 实际上修改的是旋转轴的方向向量 (默认 0,0,1)
    // 使用 "旋转轴向量" 更能准确描述物理意义，防止误以为是圆心坐标
    addParamRow(scrollLayout, "壁面旋转轴向量 X", rotationPoint1X, "-");
    addParamRow(scrollLayout, "壁面旋转轴向量 Y", rotationPoint1Y, "-");
    addParamRow(scrollLayout, "壁面旋转轴向量 Z", rotationPoint1Z, "-");

    addSaveButton(scrollLayout, "保存边界条件", [this](){
        if(m_saveCallback) {
            ExtrusionBoundaryData d;
            getData(d);
            m_saveCallback(d);
        }
    });
    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void ExtrusionBoundaryWidget::setSaveCallback(std::function<void(const ExtrusionBoundaryData&)> cb) {
    m_saveCallback = cb;
}

void ExtrusionBoundaryWidget::getData(ExtrusionBoundaryData &d) const {
    d.wallUpTemp = wallUpTemp->text();
    d.wallTemp = wallTemp->text();
    d.wallDownTemp = wallDownTemp->text();
    d.rotationPoint1X = rotationPoint1X->text();
    d.rotationPoint1Y = rotationPoint1Y->text();
    d.rotationPoint1Z = rotationPoint1Z->text();
}

void ExtrusionBoundaryWidget::setData(const ExtrusionBoundaryData &d) {
    wallUpTemp->setText(d.wallUpTemp);
    wallTemp->setText(d.wallTemp);
    wallDownTemp->setText(d.wallDownTemp);
    rotationPoint1X->setText(d.rotationPoint1X);
    rotationPoint1Y->setText(d.rotationPoint1Y);
    rotationPoint1Z->setText(d.rotationPoint1Z);
}
