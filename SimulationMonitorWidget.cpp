#include "SimulationMonitorWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>

SimulationMonitorWidget::SimulationMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    statusLabel = new QLabel(QStringLiteral("等待仿真"), this);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->hide();

    logEdit = new QTextEdit(this);
    logEdit->setReadOnly(true);

    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
    layout->addWidget(logEdit);
}

void SimulationMonitorWidget::appendLog(const QString &text)
{
    logEdit->append(text);
}

void SimulationMonitorWidget::setStatus(const QString &text)
{
    statusLabel->setText(text);
}

void SimulationMonitorWidget::setProgress(int value)
{
    progressBar->setValue(value);
}
