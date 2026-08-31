#include "StructureParamWidget.h"

#include <QScrollArea>

StructureParamWidget::StructureParamWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void StructureParamWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("结构参数"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));

    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    radiusEdit = createSciEdit();
    heightEdit = createSciEdit();
    shellThicknessEdit = createSciEdit();

    addParamRow(
        scrollLayout,
        QStringLiteral("药柱半径（mm）"),
        radiusEdit
    );
    addParamRow(
        scrollLayout,
        QStringLiteral("药柱高度（mm）"),
        heightEdit
    );
    addParamRow(
        scrollLayout,
        QStringLiteral("外壳厚度（mm）"),
        shellThicknessEdit
    );

    addSaveButton(scrollLayout, QStringLiteral("保存结构参数"), [this]() {
        emit saveRequested();
    });

    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);

    setConfig(StructureConfig());
}

void StructureParamWidget::setConfig(const StructureConfig &config)
{
    radiusEdit->setText(QString::number(config.chargeRadius, 'g', 15));
    heightEdit->setText(QString::number(config.chargeHeight, 'g', 15));
    shellThicknessEdit->setText(QString::number(config.shellThickness, 'g', 15));
}

StructureConfig StructureParamWidget::getConfig() const
{
    StructureConfig config;
    config.chargeRadius = radiusEdit->text().trimmed().toDouble();
    config.chargeHeight = heightEdit->text().trimmed().toDouble();
    config.shellThickness = shellThicknessEdit->text().trimmed().toDouble();
    return config;
}
