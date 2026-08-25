#include "SimulationMonitorWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QFrame>

namespace {

const char *kCardStyle =
    "QFrame {"
    "  background-color: #ffffff;"
    "  border: 1px solid #e8e8e8;"
    "  border-radius: 8px;"
    "}";

const char *kSectionTitleStyle =
    "font-family: 'Microsoft YaHei';"
    "font-size: 16px;"
    "font-weight: bold;"
    "color: #333333;"
    "border: none;"
    "background: transparent;";

const char *kBodyLabelStyle =
    "font-family: 'Microsoft YaHei';"
    "font-size: 16px;"
    "color: #555555;"
    "border: none;"
    "background: transparent;";

} // namespace

QFrame *SimulationMonitorWidget::createCard()
{
    QFrame *card = new QFrame(this);
    card->setObjectName(QStringLiteral("monitorCard"));
    card->setStyleSheet(QString::fromUtf8(kCardStyle));
    return card;
}

SimulationMonitorWidget::SimulationMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        QStringLiteral(
            "SimulationMonitorWidget {"
            "  background-color: #ffffff;"
            "}"
        )
    );

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    // ---------- 标题 ----------
    titleLabel = new QLabel(
        QStringLiteral("Abaqus固化仿真监控"),
        this
    );
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 20px;"
            "font-weight: bold;"
            "color: #333333;"
            "padding: 10px 0 10px 4px;"
        )
    );
    layout->addWidget(titleLabel);

    // ---------- 当前状态卡片 ----------
    QFrame *statusCard = createCard();
    QVBoxLayout *statusLayout = new QVBoxLayout(statusCard);
    statusLayout->setContentsMargins(16, 14, 16, 14);
    statusLayout->setSpacing(10);

    QLabel *statusTitle = new QLabel(QStringLiteral("当前状态"), statusCard);
    statusTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    statusValueLabel = new QLabel(QStringLiteral("● 等待仿真"), statusCard);
    statusValueLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 18px;"
            "font-weight: bold;"
            "color: #1890ff;"
            "border: none;"
            "background: transparent;"
        )
    );

    statusLayout->addWidget(statusTitle);
    statusLayout->addWidget(statusValueLabel);
    layout->addWidget(statusCard);

    // ---------- 阶段卡片 ----------
    QFrame *phaseCard = createCard();
    QVBoxLayout *phaseLayout = new QVBoxLayout(phaseCard);
    phaseLayout->setContentsMargins(16, 14, 16, 14);
    phaseLayout->setSpacing(8);

    QLabel *phaseTitle = new QLabel(QStringLiteral("当前阶段"), phaseCard);
    phaseTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    phaseValueLabel = new QLabel(QStringLiteral("空闲"), phaseCard);
    phaseValueLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 16px;"
            "color: #333333;"
            "border: none;"
            "background: transparent;"
        )
    );

    jobValueLabel = new QLabel(QStringLiteral("Job: -"), phaseCard);
    jobValueLabel->setStyleSheet(QString::fromUtf8(kBodyLabelStyle));

    phaseLayout->addWidget(phaseTitle);
    phaseLayout->addWidget(phaseValueLabel);
    phaseLayout->addWidget(jobValueLabel);
    layout->addWidget(phaseCard);

    // ---------- 进度 ----------
    QFrame *progressCard = createCard();
    QVBoxLayout *progressLayout = new QVBoxLayout(progressCard);
    progressLayout->setContentsMargins(16, 14, 16, 14);
    progressLayout->setSpacing(10);

    QLabel *progressTitle = new QLabel(QStringLiteral("进度"), progressCard);
    progressTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    progressBar = new QProgressBar(progressCard);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedHeight(26);
    progressBar->setFormat(QStringLiteral("Abaqus分析进度 %p%"));
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar {"
            "  border: 1px solid #d9d9d9;"
            "  border-radius: 6px;"
            "  text-align: center;"
            "  font-family: 'Microsoft YaHei';"
            "  font-size: 16px;"
            "  background-color: #f5f5f5;"
            "  color: #333333;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #1890ff;"
            "  border-radius: 6px;"
            "}"
        )
    );

    progressLayout->addWidget(progressTitle);
    progressLayout->addWidget(progressBar);
    layout->addWidget(progressCard);

    // ---------- 日志 ----------
    QFrame *logCard = createCard();
    QVBoxLayout *logLayout = new QVBoxLayout(logCard);
    logLayout->setContentsMargins(16, 14, 16, 14);
    logLayout->setSpacing(10);

    QLabel *logTitle = new QLabel(QStringLiteral("Abaqus日志"), logCard);
    logTitle->setStyleSheet(QString::fromUtf8(kSectionTitleStyle));

    logEdit = new QTextEdit(logCard);
    logEdit->setReadOnly(true);
    logEdit->document()->setMaximumBlockCount(10000);
    logEdit->setMinimumHeight(220);
    logEdit->setStyleSheet(
        QStringLiteral(
            "QTextEdit {"
            "  background-color: #fafafa;"
            "  border: 1px solid #dddddd;"
            "  border-radius: 6px;"
            "  font-family: Consolas, 'Courier New', monospace;"
            "  font-size: 14px;"
            "  padding: 8px;"
            "  color: #333333;"
            "}"
        )
    );

    logLayout->addWidget(logTitle);
    logLayout->addWidget(logEdit, 1);
    layout->addWidget(logCard, 1);
}

void SimulationMonitorWidget::appendLog(const QString &text)
{
    logEdit->append(text);
}

void SimulationMonitorWidget::clearLog()
{
    // 新一轮实时仿真时恢复日志上限，
    // 防止长时间运行造成界面日志无限增长。
    logEdit->document()->setMaximumBlockCount(10000);
    logEdit->clear();
}

void SimulationMonitorWidget::setLogText(const QString &text)
{
    // 查看历史日志时不限制行数，完整显示 readAll() 的内容。
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

void SimulationMonitorWidget::setPhase(const QString &text)
{
    QString value = text.trimmed();
    if (value.startsWith(QStringLiteral("阶段:"))) {
        value = value.mid(QStringLiteral("阶段:").size()).trimmed();
    } else if (value.startsWith(QStringLiteral("阶段："))) {
        value = value.mid(QStringLiteral("阶段：").size()).trimmed();
    }
    phaseValueLabel->setText(value.isEmpty() ? QStringLiteral("空闲") : value);
}

void SimulationMonitorWidget::setJob(const QString &jobName)
{
    if (jobName.trimmed().isEmpty()) {
        jobValueLabel->setText(QStringLiteral("Job: -"));
        return;
    }
    jobValueLabel->setText(QStringLiteral("Job: %1").arg(jobName));
}

void SimulationMonitorWidget::setProgress(int value)
{
    progressBar->setValue(qBound(0, value, 100));
}
