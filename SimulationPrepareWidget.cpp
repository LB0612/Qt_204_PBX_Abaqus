#include "SimulationPrepareWidget.h"

#include <QFrame>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
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
    : BaseParamWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);

    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    setPalette(palette);

    applyCommonStyles();

    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("Abaqus仿真准备"));

    QScrollArea *scrollArea = createScrollArea(this);
    scrollArea->setStyleSheet(
        QStringLiteral(
            "QScrollArea {"
            "  background-color: #ffffff;"
            "  border: none;"
            "}"
        )
    );

    QWidget *content = new QWidget();
    content->setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    QVBoxLayout *contentLayout = createScrollContentLayout(content);

    statusCard = new QFrame(content);
    statusCard->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Maximum
    );
    statusCard->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            "  background-color: #ffffff;"
            "  border: 1px solid #e0e0e0;"
            "  border-radius: 4px;"
            "}"
        )
    );

    QVBoxLayout *cardLayout = new QVBoxLayout(statusCard);
    cardLayout->setContentsMargins(20, 16, 20, 16);
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

    startButton = new QPushButton(QStringLiteral("开始计算"), statusCard);
    startButton->setFixedHeight(42);
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: %1;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  color: #000000;"
            "  background-color: #ffffff;"
            "  border: 1px solid #bfbfbf;"
            "  border-radius: 4px;"
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

    cardLayout->addSpacing(12);
    cardLayout->addWidget(
        startButton,
        0,
        Qt::AlignLeft
    );

    contentLayout->addWidget(statusCard);
    contentLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);

    connect(
        startButton,
        &QPushButton::clicked,
        this,
        &SimulationPrepareWidget::startRequested
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
