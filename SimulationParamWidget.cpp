#include "SimulationParamWidget.h"

#include <QScrollArea>

SimulationParamWidget::SimulationParamWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void SimulationParamWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("仿真设置"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));

    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    timeLengthEdit = createSciEdit();

    addParamRow(
        scrollLayout,
        QStringLiteral("时间长度（s）"),
        timeLengthEdit
    );

    addSaveButton(scrollLayout, QStringLiteral("保存仿真设置"), [this]() {
        emit saveRequested();
    });

    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);

    setConfig(SimulationConfig());
}

void SimulationParamWidget::setConfig(const SimulationConfig &config)
{
    timeLengthEdit->setText(QString::number(config.timeLength, 'g', 15));
}

SimulationConfig SimulationParamWidget::getConfig() const
{
    SimulationConfig config;
    config.timeLength = timeLengthEdit->text().trimmed().toDouble();
    config.schemaVersion = 1;
    return config;
}
