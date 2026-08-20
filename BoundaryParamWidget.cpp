#include "BoundaryParamWidget.h"

#include <QScrollArea>

BoundaryParamWidget::BoundaryParamWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void BoundaryParamWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("边界条件"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));

    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    ambientTemperatureEdit = createSciEdit();

    addParamRow(
        scrollLayout,
        QStringLiteral("环境温度（K）"),
        ambientTemperatureEdit
    );

    addSaveButton(scrollLayout, QStringLiteral("保存边界条件"), [this]() {
        emit saveRequested();
    });

    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);

    setConfig(BoundaryConfig());
}

void BoundaryParamWidget::setConfig(const BoundaryConfig &config)
{
    ambientTemperatureEdit->setText(QString::number(config.ambientTemperature, 'g', 15));
}

BoundaryConfig BoundaryParamWidget::getConfig() const
{
    BoundaryConfig config;
    config.ambientTemperature = ambientTemperatureEdit->text().trimmed().toDouble();
    config.schemaVersion = 1;
    return config;
}
