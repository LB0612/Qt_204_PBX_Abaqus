#include "SimulationMonitorWidget.h"

#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

const char *kUiFontFamily =
    "\"Microsoft YaHei UI\", \"Microsoft YaHei\"";

const char *kSectionTitleStyle =
    "font-family: 'Microsoft YaHei UI', 'Microsoft YaHei';"
    "font-size: 16px;"
    "font-weight: bold;"
    "color: #333333;"
    "border: none;"
    "background: transparent;";

void applyOpaqueWhitePage(QWidget *widget)
{
    widget->setAttribute(Qt::WA_StyledBackground, true);
    widget->setAutoFillBackground(true);

    QPalette palette = widget->palette();
    palette.setColor(QPalette::Window, Qt::white);
    widget->setPalette(palette);

    widget->setStyleSheet(
        QStringLiteral("background-color: #ffffff;")
    );
}

void applyOpaqueWhiteScrollArea(QScrollArea *scrollArea)
{
    scrollArea->setStyleSheet(
        QStringLiteral(
            "QScrollArea {"
            " background-color: #ffffff;"
            " border: none;"
            "}"
            "QScrollArea > QWidget > QWidget {"
            " background-color: #ffffff;"
            "}"
        )
    );
    scrollArea->viewport()->setStyleSheet(
        QStringLiteral("background-color: #ffffff;")
    );
}

} // namespace

SimulationMonitorWidget::SimulationMonitorWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    applyOpaqueWhitePage(this);
    applyCommonStyles();

    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("Abaqus固化仿真监控"), false);

    QScrollArea *scrollArea = createScrollArea(this);
    applyOpaqueWhiteScrollArea(scrollArea);

    QWidget *content = new QWidget();
    content->setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    QVBoxLayout *layout = createScrollContentLayout(content);

    QLabel *statusTitle = new QLabel(
        QStringLiteral("当前状态"),
        content
    );
    statusTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    statusValueLabel = new QLabel(
        QStringLiteral("● 等待仿真"),
        content
    );
    statusValueLabel->setStyleSheet(
        QStringLiteral(
            "font-family: %1;"
            "font-size: 16px;"
            "font-weight: 400;"
            "color: #000000;"
            "border: none;"
            "background: transparent;"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    QLabel *progressTitle = new QLabel(
        QStringLiteral("仿真进度"),
        content
    );
    progressTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    progressBar = new QProgressBar(content);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedHeight(26);
    progressBar->setFormat(QStringLiteral("Abaqus分析进度 %p%"));
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar {"
            "  border: 1px solid #dddddd;"
            "  border-radius: 4px;"
            "  text-align: center;"
            "  font-family: 'Microsoft YaHei UI', 'Microsoft YaHei';"
            "  font-size: 16px;"
            "  background-color: #ffffff;"
            "  color: #000000;"
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
    logTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    logEdit = new QTextEdit(content);
    logEdit->setReadOnly(true);
    logEdit->document()->setMaximumBlockCount(10000);
    logEdit->setMinimumHeight(280);
    logEdit->setStyleSheet(
        QStringLiteral(
            "QTextEdit {"
            "  background-color: #ffffff;"
            "  border: 1px solid #dddddd;"
            "  border-radius: 4px;"
            "  font-family: Consolas, 'Courier New', monospace;"
            "  font-size: 14px;"
            "  padding: 8px;"
            "  color: #000000;"
            "}"
        )
    );

    layout->addWidget(statusTitle);
    layout->addWidget(statusValueLabel);
    layout->addSpacing(8);
    layout->addWidget(progressTitle);
    layout->addWidget(progressBar);
    layout->addSpacing(8);
    layout->addWidget(logTitle);
    layout->addWidget(logEdit, 1);
    layout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void SimulationMonitorWidget::appendLog(const QString &text)
{
    logEdit->append(text);
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
    progressBar->setValue(qBound(0, value, 100));
}
