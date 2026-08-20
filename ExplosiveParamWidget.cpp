#include "ExplosiveParamWidget.h"

#include <QScrollArea>

ExplosiveParamWidget::ExplosiveParamWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void ExplosiveParamWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("炸药参数"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));

    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    densityEdit = createSciEdit();
    initialElasticModulusEdit = createSciEdit();
    initialPoissonRatioEdit = createSciEdit();
    finalElasticModulusEdit = createSciEdit();
    finalPoissonRatioEdit = createSciEdit();
    thermalConductivityEdit = createSciEdit();
    yieldStressEdit = createSciEdit();
    specificHeatEdit = createSciEdit();
    expansionCoefficientEdit = createSciEdit();

    addParamRow(scrollLayout, QStringLiteral("炸药密度"), densityEdit);
    addParamRow(scrollLayout, QStringLiteral("固化初始杨氏模量"), initialElasticModulusEdit);
    addParamRow(scrollLayout, QStringLiteral("固化初始泊松比"), initialPoissonRatioEdit);
    addParamRow(scrollLayout, QStringLiteral("固化结束杨氏模量"), finalElasticModulusEdit);
    addParamRow(scrollLayout, QStringLiteral("固化结束泊松比"), finalPoissonRatioEdit);
    addParamRow(scrollLayout, QStringLiteral("炸药传导率"), thermalConductivityEdit);
    addParamRow(scrollLayout, QStringLiteral("炸药屈服应力"), yieldStressEdit);
    addParamRow(scrollLayout, QStringLiteral("炸药比热"), specificHeatEdit);
    addParamRow(scrollLayout, QStringLiteral("炸药膨胀系数"), expansionCoefficientEdit);

    addSaveButton(scrollLayout, QStringLiteral("保存炸药参数"), [this]() {
        emit saveRequested();
    });

    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);

    setConfig(ExplosiveConfig());
}

void ExplosiveParamWidget::setConfig(const ExplosiveConfig &config)
{
    densityEdit->setText(QString::number(config.density, 'g', 15));
    initialElasticModulusEdit->setText(QString::number(config.initialElasticModulus, 'g', 15));
    initialPoissonRatioEdit->setText(QString::number(config.initialPoissonRatio, 'g', 15));
    finalElasticModulusEdit->setText(QString::number(config.finalElasticModulus, 'g', 15));
    finalPoissonRatioEdit->setText(QString::number(config.finalPoissonRatio, 'g', 15));
    thermalConductivityEdit->setText(QString::number(config.thermalConductivity, 'g', 15));
    yieldStressEdit->setText(QString::number(config.yieldStress, 'g', 15));
    specificHeatEdit->setText(QString::number(config.specificHeat, 'g', 15));
    expansionCoefficientEdit->setText(QString::number(config.expansionCoefficient, 'g', 15));
}

ExplosiveConfig ExplosiveParamWidget::getConfig() const
{
    ExplosiveConfig config;
    config.density = densityEdit->text().trimmed().toDouble();
    config.initialElasticModulus = initialElasticModulusEdit->text().trimmed().toDouble();
    config.initialPoissonRatio = initialPoissonRatioEdit->text().trimmed().toDouble();
    config.finalElasticModulus = finalElasticModulusEdit->text().trimmed().toDouble();
    config.finalPoissonRatio = finalPoissonRatioEdit->text().trimmed().toDouble();
    config.thermalConductivity = thermalConductivityEdit->text().trimmed().toDouble();
    config.yieldStress = yieldStressEdit->text().trimmed().toDouble();
    config.specificHeat = specificHeatEdit->text().trimmed().toDouble();
    config.expansionCoefficient = expansionCoefficientEdit->text().trimmed().toDouble();
    config.schemaVersion = 1;
    return config;
}
