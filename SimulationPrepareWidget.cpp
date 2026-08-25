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
        "color: #000000;"
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
        "color: #000000;"
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
        "color: #000000;"
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
        "color: #000000;"
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
        "color: #000000;"
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
            "color: #000000;"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    statusCard = new QFrame(this);
    statusCard->setMinimumWidth(600);
    statusCard->setMaximumWidth(800);
    statusCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    statusCard->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "  background-color: #ffffff;"
            "  border: 1px solid #dddddd;"
            "  border-radius: 6px;"
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

    startButton = new QPushButton(QStringLiteral("开始计算"), statusCard);
    startButton->setMinimumHeight(42);
    startButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: %1;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  color: #000000;"
            "  background-color: #ffffff;"
            "  border: 1px solid #bfbfbf;"
            "  border-radius: 6px;"
            "  padding: 8px 28px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f2f2f2;"
            "  border-color: #888888;"
            "}"
            "QPushButton:disabled {"
            "  color: #999999;"
            "  background-color: #f5f5f5;"
            "  border-color: #dddddd;"
            "}"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    cancelButton = new QPushButton(QStringLiteral("取消"), statusCard);
    cancelButton->setMinimumHeight(42);
    cancelButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: %1;"
            "  font-size: 16px;"
            "  color: #000000;"
            "  background-color: #ffffff;"
            "  border: 1px solid #bfbfbf;"
            "  border-radius: 6px;"
            "  padding: 8px 28px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f2f2f2;"
            "  border-color: #888888;"
            "}"
        ).arg(QString::fromUtf8(kUiFontFamily))
    );

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 12, 0, 0);
    buttonLayout->setSpacing(12);
    buttonLayout->addStretch();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();
    cardLayout->addLayout(buttonLayout);

    QHBoxLayout *cardCenterLayout = new QHBoxLayout();
    cardCenterLayout->addStretch();
    cardCenterLayout->addWidget(statusCard);
    cardCenterLayout->addStretch();

    layout->addWidget(titleLabel);
    layout->addSpacing(16);
    layout->addLayout(cardCenterLayout);
    layout->addStretch();

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
