#include "SimulationMonitorWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QPlainTextEdit>
#include <QVBoxLayout>

SimulationMonitorWidget::SimulationMonitorWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    applyCommonStyles();

    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("Abaqus固化仿真监控"), false);

    QScrollArea *scrollArea = createScrollArea(this);

    QWidget *content = new QWidget();
    content->setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    QVBoxLayout *layout = createScrollContentLayout(content);

    QLabel *statusTitle = new QLabel(
        QStringLiteral("当前状态"),
        content
    );
    statusTitle->setStyleSheet(
        QStringLiteral(
            "font-family: %1;"
            "font-size: 18px;"
            "font-weight: bold;"
            "color: #222222;"
            "border: none;"
            "background: transparent;"
        ).arg(uiFontFamily())
    );

    statusValueLabel = new QLabel(
        QStringLiteral("● 等待仿真"),
        content
    );
    statusValueLabel->setStyleSheet(
        QStringLiteral(
            "font-family: %1;"
            "font-size: 18px;"
            "font-weight: bold;"
            "color: #000000;"
            "border: none;"
            "background: transparent;"
        ).arg(uiFontFamily())
    );

    QLabel *progressTitle = new QLabel(
        QStringLiteral("仿真进度"),
        content
    );
    progressTitle->setStyleSheet(sectionTitleStyle());

    progressValueLabel = new QLabel(QStringLiteral("0%"), content);
    progressValueLabel->setStyleSheet(
        QStringLiteral(
            "font-family: %1;"
            "font-size: 16px;"
            "font-weight: 400;"
            "color: #000000;"
            "border: none;"
            "background: transparent;"
        ).arg(uiFontFamily())
    );
    progressValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout *progressHeaderLayout = new QHBoxLayout();
    progressHeaderLayout->setContentsMargins(0, 0, 0, 0);
    progressHeaderLayout->setSpacing(8);
    progressHeaderLayout->addWidget(progressTitle);
    progressHeaderLayout->addStretch();
    progressHeaderLayout->addWidget(progressValueLabel);

    progressBar = new QProgressBar(content);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedHeight(26);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar {"
            "  border: 1px solid #dddddd;"
            "  border-radius: 4px;"
            "  background-color: #ffffff;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #333333;"
            "  border-radius: 4px;"
            "}"
        )
    );

    QLabel *logTitle = new QLabel(
        QStringLiteral("Abaqus日志"),
        content
    );
    logTitle->setStyleSheet(sectionTitleStyle());

    logEdit = new QPlainTextEdit(content);
    logEdit->setReadOnly(true);
    logEdit->document()->setMaximumBlockCount(10000);
    logEdit->setMinimumHeight(280);
    logEdit->setStyleSheet(
        QStringLiteral(
            "QPlainTextEdit {"
            "  background-color: #ffffff;"
            "  border: 1px solid #dddddd;"
            "  border-radius: 4px;"
            "  font-family: Consolas, 'Courier New', monospace;"
            "  font-size: 16px;"
            "  padding: 8px;"
            "  color: #000000;"
            "}"
        )
    );

    layout->addWidget(statusTitle);
    layout->addWidget(statusValueLabel);
    layout->addSpacing(14);
    layout->addLayout(progressHeaderLayout);
    layout->addWidget(progressBar);
    layout->addSpacing(14);
    layout->addWidget(logTitle);
    layout->addWidget(logEdit, 1);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void SimulationMonitorWidget::appendLog(const QString &text)
{
    logEdit->appendPlainText(text);
}

void SimulationMonitorWidget::clearLog()
{
    logEdit->document()->setMaximumBlockCount(10000);
    logEdit->clear();
}

void SimulationMonitorWidget::setLogText(const QString &text)
{
    logEdit->document()->setMaximumBlockCount(0);
    logEdit->setPlainText(text);
}

void SimulationMonitorWidget::setStatus(const QString &text)
{
    QString value = text.trimmed();
    if (!value.startsWith(QStringLiteral("●"))) {
        value = QStringLiteral("● ") + value;
    }
    statusValueLabel->setText(value);
}

void SimulationMonitorWidget::setProgress(int value)
{
    const int percent = qBound(0, value, 100);
    progressBar->setValue(percent);
    progressValueLabel->setText(
        QStringLiteral("%1%").arg(percent)
    );
}
