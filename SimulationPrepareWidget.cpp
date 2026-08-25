#include "SimulationPrepareWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {

const char *kUiFontFamily =
    "\"Microsoft YaHei UI\", \"Microsoft YaHei\"";

QString sectionTitleStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 18px;"
        "font-weight: bold;"
        "color: #262626;"
        "background: transparent;"
        "border: none;"
    ).arg(QString::fromUtf8(kUiFontFamily));
}

QString checkItemStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 16px;"
        "font-weight: 400;"
        "color: #52c41a;"
        "background: transparent;"
        "border: none;"
    ).arg(QString::fromUtf8(kUiFontFamily));
}

QString hintStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 15px;"
        "font-weight: 400;"
        "color: #666666;"
        "background: transparent;"
        "border: none;"
    ).arg(QString::fromUtf8(kUiFontFamily));
}

QString errorStatusStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 16px;"
        "font-weight: 500;"
        "color: #cf1322;"
        "background: transparent;"
        "border: none;"
    ).arg(QString::fromUtf8(kUiFontFamily));
}

QString reasonBodyStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 16px;"
        "font-weight: 400;"
        "color: #454545;"
        "background: transparent;"
        "border: none;"
    ).arg(QString::fromUtf8(kUiFontFamily));
}

} // namespace

SimulationPrepareWidget::SimulationPrepareWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        QStringLiteral(
            "SimulationPrepareWidget {"
            "  background-color: #ffffff;"
            "}"
        )
    );

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);

    titleLabel = new QLabel(QStringLiteral("Abaqus仿真准备"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        QStringLiteral(
            "font-family: %1;"
            "font-size: 26px;"
            "font-weight: bold;"
            "color: #333333;"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    statusCard = new QFrame(this);
    statusCard->setMaximumWidth(900);
    statusCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    statusCard->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "  background-color: #fafafa;"
            "  border: 1px solid #e8e8e8;"
            "  border-radius: 8px;"
            "}"
        )
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(statusCard);
    cardLayout->setContentsMargins(24, 20, 24, 20);
    cardLayout->setSpacing(8);

    statusTitleLabel = new QLabel(
        QStringLiteral("工程状态"),
        statusCard
    );
    statusTitleLabel->setStyleSheet(sectionTitleStyle());

    checkParamLabel = new QLabel(
        QStringLiteral("✓ 参数配置完整"),
        statusCard
    );
    checkParamLabel->setStyleSheet(checkItemStyle());

    checkFilesLabel = new QLabel(
        QStringLiteral("✓ Abaqus文件完整"),
        statusCard
    );
    checkFilesLabel->setStyleSheet(checkItemStyle());

    checkPathLabel = new QLabel(
        QStringLiteral("✓ Abaqus路径有效"),
        statusCard
    );
    checkPathLabel->setStyleSheet(checkItemStyle());

    hintLabel = new QLabel(
        QStringLiteral("确认后即可开始计算"),
        statusCard
    );
    hintLabel->setStyleSheet(hintStyle());

    errorStatusLabel = new QLabel(
        QStringLiteral("✕ 当前不能开始仿真"),
        statusCard
    );
    errorStatusLabel->setStyleSheet(errorStatusStyle());

    reasonTitleLabel = new QLabel(
        QStringLiteral("原因"),
        statusCard
    );
    reasonTitleLabel->setStyleSheet(sectionTitleStyle());

    reasonContentLabel = new QLabel(statusCard);
    reasonContentLabel->setWordWrap(true);
    reasonContentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    reasonContentLabel->setStyleSheet(reasonBodyStyle());

    cardLayout->addWidget(statusTitleLabel);
    cardLayout->addWidget(checkParamLabel);
    cardLayout->addWidget(checkFilesLabel);
    cardLayout->addWidget(checkPathLabel);
    cardLayout->addWidget(hintLabel);
    cardLayout->addWidget(errorStatusLabel);
    cardLayout->addWidget(reasonTitleLabel);
    cardLayout->addWidget(reasonContentLabel);
    cardLayout->addSpacing(4);

    startButton = new QPushButton(QStringLiteral("开始计算"), this);
    startButton->setMinimumHeight(42);
    startButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: %1;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  color: white;"
            "  background-color: #1890ff;"
            "  border: none;"
            "  border-radius: 6px;"
            "  padding: 8px 24px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #40a9ff;"
            "}"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    cancelButton = new QPushButton(QStringLiteral("取消"), this);
    cancelButton->setMinimumHeight(42);
    cancelButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: %1;"
            "  font-size: 16px;"
            "  color: #333333;"
            "  background-color: #ffffff;"
            "  border: 1px solid #d9d9d9;"
            "  border-radius: 6px;"
            "  padding: 8px 24px;"
            "}"
            "QPushButton:hover {"
            "  border-color: #1890ff;"
            "  color: #1890ff;"
            "}"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    QHBoxLayout *cardCenterLayout = new QHBoxLayout();
    cardCenterLayout->addStretch();
    cardCenterLayout->addWidget(statusCard);
    cardCenterLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    layout->addWidget(titleLabel);
    layout->addLayout(cardCenterLayout);
    layout->addStretch();
    layout->addLayout(buttonLayout);

    connect(
        startButton,
        &QPushButton::clicked,
        this,
        &SimulationPrepareWidget::startRequested
    );
    connect(
        cancelButton,
        &QPushButton::clicked,
        this,
        &SimulationPrepareWidget::cancelRequested
    );

    setReadyState(false, QStringLiteral("请先完成参数配置。"));
}

void SimulationPrepareWidget::setReadyState(
    bool ready,
    const QString &message)
{
    if (ready) {
        checkParamLabel->show();
        checkFilesLabel->show();
        checkPathLabel->show();
        hintLabel->show();
        errorStatusLabel->hide();
        reasonTitleLabel->hide();
        reasonContentLabel->hide();
    } else {
        checkParamLabel->hide();
        checkFilesLabel->hide();
        checkPathLabel->hide();
        hintLabel->hide();
        errorStatusLabel->show();
        reasonTitleLabel->show();
        reasonContentLabel->setText(message);
        reasonContentLabel->show();
    }

    startButton->setEnabled(ready);
}
