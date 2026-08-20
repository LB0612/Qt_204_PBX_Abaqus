#include "MoldParamWidget.h"

#include <QScrollArea>

MoldParamWidget::MoldParamWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void MoldParamWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("模具参数"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));

    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    densityEdit = createSciEdit();
    elasticModulusEdit = createSciEdit();
    poissonRatioEdit = createSciEdit();
    thermalConductivityEdit = createSciEdit();
    specificHeatEdit = createSciEdit();

    addParamRow(scrollLayout, QStringLiteral("模具密度"), densityEdit);
    addParamRow(scrollLayout, QStringLiteral("模具弹性模量"), elasticModulusEdit);
    addParamRow(scrollLayout, QStringLiteral("模具泊松比"), poissonRatioEdit);
    addParamRow(scrollLayout, QStringLiteral("模具热导率"), thermalConductivityEdit);
    addParamRow(scrollLayout, QStringLiteral("模具比热容"), specificHeatEdit);

    addSaveButton(scrollLayout, QStringLiteral("保存模具参数"), [this]() {
        emit saveRequested();
    });

    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);

    setConfig(MoldConfig());
}

void MoldParamWidget::setConfig(const MoldConfig &config)
{
    densityEdit->setText(QString::number(config.density, 'g', 15));
    elasticModulusEdit->setText(QString::number(config.elasticModulus, 'g', 15));
    poissonRatioEdit->setText(QString::number(config.poissonRatio, 'g', 15));
    thermalConductivityEdit->setText(QString::number(config.thermalConductivity, 'g', 15));
    specificHeatEdit->setText(QString::number(config.specificHeat, 'g', 15));
}

MoldConfig MoldParamWidget::getConfig() const
{
    MoldConfig config;
    config.density = densityEdit->text().trimmed().toDouble();
    config.elasticModulus = elasticModulusEdit->text().trimmed().toDouble();
    config.poissonRatio = poissonRatioEdit->text().trimmed().toDouble();
    config.thermalConductivity = thermalConductivityEdit->text().trimmed().toDouble();
    config.specificHeat = specificHeatEdit->text().trimmed().toDouble();
    config.schemaVersion = 1;
    return config;
}
