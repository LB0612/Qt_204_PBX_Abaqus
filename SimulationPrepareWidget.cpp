#include "SimulationPrepareWidget.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QString sectionTitleStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 18px;"
        "font-weight: bold;"
        "color: #333333;"
        "background: transparent;"
        "border: none;"
    ).arg(BaseParamWidget::uiFontFamily());
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
    ).arg(BaseParamWidget::uiFontFamily());
}

QString hintStyle()
{
    return QStringLiteral(
        "font-family: %1;"
        "font-size: 16px;"
        "font-weight: 400;"
        "color: #000000;"
        "background: transparent;"
        "border: none;"
    ).arg(BaseParamWidget::uiFontFamily());
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
    ).arg(BaseParamWidget::uiFontFamily());
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
    ).arg(BaseParamWidget::uiFontFamily());
}

} // namespace

SimulationPrepareWidget::SimulationPrepareWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    applyCommonStyles();

    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("Abaqus仿真准备"), false);

    QScrollArea *scrollArea = createScrollArea(this);

    QWidget *content = new QWidget();
    content->setStyleSheet(QStringLiteral("background-color: #ffffff;"));

    QVBoxLayout *contentLayout = createScrollContentLayout(content);
    contentLayout->setSpacing(4);

    statusTitleLabel = new QLabel(
        QStringLiteral("工程状态"),
        content
    );
    statusTitleLabel->setStyleSheet(sectionTitleStyle());

    checkParamLabel = new QLabel(
        QStringLiteral("✓ 参数配置完整"),
        content
    );
    checkParamLabel->setStyleSheet(checkItemStyle());

    checkFilesLabel = new QLabel(
        QStringLiteral("✓ Abaqus文件完整"),
        content
    );
    checkFilesLabel->setStyleSheet(checkItemStyle());

    checkPathLabel = new QLabel(
        QStringLiteral("✓ Abaqus路径有效"),
        content
    );
    checkPathLabel->setStyleSheet(checkItemStyle());

    hintLabel = new QLabel(
        QStringLiteral("确认后即可开始计算"),
        content
    );
    hintLabel->setStyleSheet(hintStyle());

    errorStatusLabel = new QLabel(
        QStringLiteral("✕ 当前不能开始仿真"),
        content
    );
    errorStatusLabel->setStyleSheet(errorStatusStyle());

    reasonTitleLabel = new QLabel(
        QStringLiteral("原因"),
        content
    );
    reasonTitleLabel->setStyleSheet(sectionTitleStyle());

    reasonContentLabel = new QLabel(content);
    reasonContentLabel->setWordWrap(true);
    reasonContentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    reasonContentLabel->setStyleSheet(reasonBodyStyle());

    QFrame *divider = new QFrame(content);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Plain);
    divider->setStyleSheet(
        QStringLiteral(
            "QFrame {"
            " border: none;"
            " border-top: 1px solid #e5e5e5;"
            " max-height: 1px;"
            "}"
        )
    );

    startButton = new QPushButton(QStringLiteral("开始计算"), content);
    startButton->setFixedSize(160, 42);
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: %1;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  color: #ffffff;"
            "  background-color: #1890ff;"
            "  border: 1px solid #1890ff;"
            "  border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #40a9ff;"
            "  border-color: #40a9ff;"
            "}"
            "QPushButton:disabled {"
            "  color: #f5f5f5;"
            "  background-color: #bfbfbf;"
            "  border-color: #bfbfbf;"
            "}"
        ).arg(uiFontFamily())
    );

    contentLayout->addWidget(statusTitleLabel);
    contentLayout->addSpacing(6);

    contentLayout->addWidget(checkParamLabel);
    contentLayout->addWidget(checkFilesLabel);
    contentLayout->addWidget(checkPathLabel);

    contentLayout->addWidget(errorStatusLabel);
    contentLayout->addWidget(reasonTitleLabel);
    contentLayout->addWidget(reasonContentLabel);

    contentLayout->addSpacing(12);
    contentLayout->addWidget(divider);
    contentLayout->addSpacing(12);

    contentLayout->addWidget(hintLabel);

    contentLayout->addSpacing(14);
    contentLayout->addWidget(startButton, 0, Qt::AlignRight);
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
