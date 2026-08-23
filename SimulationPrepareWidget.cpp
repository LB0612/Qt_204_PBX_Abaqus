#include "SimulationPrepareWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

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
    layout->setSpacing(20);

    titleLabel = new QLabel(
        QStringLiteral("Abaqus仿真准备"),
        this
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 26px;"
            "font-weight: bold;"
            "color: #333333;"
        )
    );

    infoLabel = new QLabel(
        QStringLiteral(
            "工程状态:\n"
            "✓ 参数检查\n"
            "✓ Abaqus文件\n"
            "✓ 用户子程序\n\n"
            "确认后开始计算"
        ),
        this
    );
    infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    infoLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 16px;"
            "color: #555555;"
            "line-height: 1.6;"
            "padding: 16px;"
            "background-color: #fafafa;"
            "border: 1px solid #e8e8e8;"
            "border-radius: 8px;"
        )
    );

    startButton = new QPushButton(
        QStringLiteral("开始计算"),
        this
    );
    startButton->setMinimumHeight(42);
    startButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: 'Microsoft YaHei';"
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
        )
    );

    cancelButton = new QPushButton(
        QStringLiteral("取消"),
        this
    );
    cancelButton->setMinimumHeight(42);
    cancelButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: 'Microsoft YaHei';"
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
        )
    );

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addStretch();

    layout->addWidget(titleLabel);
    layout->addWidget(infoLabel, 1);
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
}

void SimulationPrepareWidget::setReadyState(
    bool ready,
    const QString &message)
{
    if (ready) {
        infoLabel->setText(
            QStringLiteral(
                "工程状态:\n"
                "✓ 参数配置完整\n"
                "✓ Abaqus文件完整\n"
                "✓ Abaqus路径有效\n\n"
                "确认后开始计算"
            )
        );
    } else {
        infoLabel->setText(
            QStringLiteral(
                "工程状态:\n"
                "✗ 当前不能开始仿真\n\n"
                "原因:\n%1"
            ).arg(message)
        );
    }

    startButton->setEnabled(ready);
}
