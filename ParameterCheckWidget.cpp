#include "ParameterCheckWidget.h"

#include "BoundaryConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "SimulationConfigManager.h"
#include "StructureConfigManager.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QSize>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {

enum class CheckStatus {
    Pass,
    Missing,
    Invalid
};

QString statusText(CheckStatus status)
{
    switch (status) {
    case CheckStatus::Pass:
        return QStringLiteral("通过");
    case CheckStatus::Missing:
        return QStringLiteral("未保存");
    case CheckStatus::Invalid:
        return QStringLiteral("文件无效");
    }
    return {};
}

QColor statusColor(CheckStatus status)
{
    switch (status) {
    case CheckStatus::Pass:
        return QColor(QStringLiteral("#389e0d"));
    case CheckStatus::Missing:
        return QColor(QStringLiteral("#d48806"));
    case CheckStatus::Invalid:
        return QColor(QStringLiteral("#cf1322"));
    }
    return QColor(Qt::black);
}

QString formatNumber(double value)
{
    return QString::number(value, 'g', 15);
}

void applyStatus(QTreeWidgetItem *item, CheckStatus status)
{
    item->setText(2, statusText(status));
    item->setForeground(2, statusColor(status));
}

QTreeWidgetItem *addGroupItem(
    QTreeWidget *tree,
    const QString &title,
    CheckStatus status)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(tree);
    item->setText(0, title);
    applyStatus(item, status);
    item->setExpanded(true);

    QFont font = item->font(0);
    font.setBold(true);

    for (int column = 0; column < 3; ++column) {
        item->setFont(column, font);
        item->setBackground(
            column,
            QBrush(QColor(QStringLiteral("#f7f9fc")))
        );
    }

    item->setSizeHint(0, QSize(0, 40));
    return item;
}

void addChildValue(
    QTreeWidgetItem *parent,
    const QString &name,
    const QString &value,
    CheckStatus childStatus = CheckStatus::Pass)
{
    QTreeWidgetItem *child = new QTreeWidgetItem(parent);
    child->setText(0, name);
    child->setText(1, value);
    if (childStatus == CheckStatus::Pass && !value.isEmpty()) {
        child->setText(2, QString());
    } else {
        applyStatus(child, childStatus);
    }
}

} // namespace

ParameterCheckWidget::ParameterCheckWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void ParameterCheckWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("参数检查"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    QFrame *card = new QFrame(content);
    card->setObjectName(QStringLiteral("CheckCard"));
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    card->setStyleSheet(R"(
        QFrame#CheckCard {
            background-color: #ffffff;
            border: 1px solid #e5e5e5;
            border-radius: 6px;
        }
    )");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 20);
    cardLayout->setSpacing(14);

    QLabel *cardTitle = new QLabel(QStringLiteral("参数检查结果"));
    cardTitle->setAlignment(Qt::AlignCenter);
    cardTitle->setStyleSheet(R"(
        QLabel {
            font-family: 'Microsoft YaHei';
            font-size: 18px;
            font-weight: bold;
            color: #333333;
            padding-bottom: 4px;
        }
    )");
    cardLayout->addWidget(cardTitle);

    summaryLabel = new QLabel();
    summaryLabel->setWordWrap(true);
    summaryLabel->setText(QStringLiteral("请打开工程后进行参数检查。"));
    summaryLabel->setStyleSheet(R"(
        QLabel {
            background-color: #f8f9fa;
            border: 1px solid #e5e5e5;
            border-radius: 5px;
            color: #666666;
            font-family: 'Microsoft YaHei';
            font-size: 16px;
            font-weight: bold;
            padding: 10px 15px;
        }
    )");
    cardLayout->addWidget(summaryLabel);

    treeWidget = new QTreeWidget();
    treeWidget->setColumnCount(3);
    treeWidget->setHeaderLabels({
        QStringLiteral("检查项"),
        QStringLiteral("当前值"),
        QStringLiteral("状态")
    });
    treeWidget->setRootIsDecorated(true);
    treeWidget->setAlternatingRowColors(false);
    treeWidget->setUniformRowHeights(false);
    treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
    treeWidget->setFocusPolicy(Qt::NoFocus);
    treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    treeWidget->setStyleSheet(R"(
        QTreeWidget {
            background-color: #ffffff;
            border: 1px solid #e5e5e5;
            border-radius: 4px;
            font-family: 'Microsoft YaHei';
            font-size: 15px;
            color: #333333;
            outline: none;
        }

        QTreeWidget::item {
            height: 36px;
            border-bottom: 1px solid #f0f0f0;
        }

        QTreeWidget::item:hover {
            background-color: #f5f9ff;
        }

        QHeaderView::section {
            background-color: #f8f9fa;
            color: #333333;
            border: none;
            border-bottom: 1px solid #dddddd;
            padding: 10px 12px;
            font-family: 'Microsoft YaHei';
            font-size: 15px;
            font-weight: bold;
        }
    )");

    QHeaderView *header = treeWidget->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    treeWidget->setColumnWidth(0, 320);
    treeWidget->setColumnWidth(2, 100);

    cardLayout->addWidget(treeWidget, 1);

    scrollLayout->addWidget(card, 1);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void ParameterCheckWidget::refresh(const ProjectConfig &project)
{
    treeWidget->clear();

    bool allPassed = true;

    // 工程信息
    {
        ProjectConfig loaded;
        const QString projectJsonPath =
            QDir(project.projectPath).filePath(QStringLiteral("project.json"));
        const bool exists = QFileInfo::exists(projectJsonPath);
        const bool valid =
            exists && ProjectManager::loadProject(project.projectPath, loaded);

        CheckStatus status = CheckStatus::Pass;
        if (!exists) {
            status = CheckStatus::Missing;
        } else if (!valid || loaded.projectName.trimmed().isEmpty()) {
            status = CheckStatus::Invalid;
        }

        if (status != CheckStatus::Pass) {
            allPassed = false;
        }

        QTreeWidgetItem *group = addGroupItem(
            treeWidget,
            QStringLiteral("工程信息"),
            status
        );

        if (status == CheckStatus::Pass) {
            addChildValue(
                group,
                QStringLiteral("项目名称"),
                loaded.projectName,
                CheckStatus::Pass
            );
            group->child(0)->setText(2, statusText(CheckStatus::Pass));
            group->child(0)->setForeground(2, statusColor(CheckStatus::Pass));
        }
    }

    // 炸药参数
    {
        ExplosiveConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/explosive.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && ExplosiveConfigManager::load(project.projectPath, config);

        CheckStatus status = CheckStatus::Pass;
        if (!exists) {
            status = CheckStatus::Missing;
        } else if (!valid) {
            status = CheckStatus::Invalid;
        }

        if (status != CheckStatus::Pass) {
            allPassed = false;
        }

        QTreeWidgetItem *group = addGroupItem(
            treeWidget,
            QStringLiteral("炸药参数"),
            status
        );

        if (status == CheckStatus::Pass) {
            addChildValue(group, QStringLiteral("炸药密度"), formatNumber(config.density));
            addChildValue(group, QStringLiteral("固化初始杨氏模量"), formatNumber(config.initialElasticModulus));
            addChildValue(group, QStringLiteral("固化初始泊松比"), formatNumber(config.initialPoissonRatio));
            addChildValue(group, QStringLiteral("固化结束杨氏模量"), formatNumber(config.finalElasticModulus));
            addChildValue(group, QStringLiteral("固化结束泊松比"), formatNumber(config.finalPoissonRatio));
            addChildValue(group, QStringLiteral("炸药传导率"), formatNumber(config.thermalConductivity));
            addChildValue(group, QStringLiteral("炸药屈服应力"), formatNumber(config.yieldStress));
            addChildValue(group, QStringLiteral("炸药比热"), formatNumber(config.specificHeat));
            addChildValue(group, QStringLiteral("炸药膨胀系数"), formatNumber(config.expansionCoefficient));
        }
    }

    // 结构参数
    {
        StructureConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/structure.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && StructureConfigManager::load(project.projectPath, config);

        CheckStatus status = CheckStatus::Pass;
        if (!exists) {
            status = CheckStatus::Missing;
        } else if (!valid) {
            status = CheckStatus::Invalid;
        }

        if (status != CheckStatus::Pass) {
            allPassed = false;
        }

        QTreeWidgetItem *group = addGroupItem(
            treeWidget,
            QStringLiteral("结构参数"),
            status
        );

        if (status == CheckStatus::Pass) {
            addChildValue(group, QStringLiteral("药柱半径"), formatNumber(config.chargeRadius));
            addChildValue(group, QStringLiteral("药柱高度"), formatNumber(config.chargeHeight));
            addChildValue(group, QStringLiteral("外壳厚度"), formatNumber(config.shellThickness));
        }
    }

    // 模具参数
    {
        MoldConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/mold.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && MoldConfigManager::load(project.projectPath, config);

        CheckStatus status = CheckStatus::Pass;
        if (!exists) {
            status = CheckStatus::Missing;
        } else if (!valid) {
            status = CheckStatus::Invalid;
        }

        if (status != CheckStatus::Pass) {
            allPassed = false;
        }

        QTreeWidgetItem *group = addGroupItem(
            treeWidget,
            QStringLiteral("模具参数"),
            status
        );

        if (status == CheckStatus::Pass) {
            addChildValue(group, QStringLiteral("模具密度"), formatNumber(config.density));
            addChildValue(group, QStringLiteral("模具弹性模量"), formatNumber(config.elasticModulus));
            addChildValue(group, QStringLiteral("模具泊松比"), formatNumber(config.poissonRatio));
            addChildValue(group, QStringLiteral("模具热导率"), formatNumber(config.thermalConductivity));
            addChildValue(group, QStringLiteral("模具比热容"), formatNumber(config.specificHeat));
        }
    }

    // 边界条件
    {
        BoundaryConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/boundary.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && BoundaryConfigManager::load(project.projectPath, config);

        CheckStatus status = CheckStatus::Pass;
        if (!exists) {
            status = CheckStatus::Missing;
        } else if (!valid) {
            status = CheckStatus::Invalid;
        }

        if (status != CheckStatus::Pass) {
            allPassed = false;
        }

        QTreeWidgetItem *group = addGroupItem(
            treeWidget,
            QStringLiteral("边界条件"),
            status
        );

        if (status == CheckStatus::Pass) {
            addChildValue(
                group,
                QStringLiteral("环境温度"),
                formatNumber(config.ambientTemperature) + QStringLiteral(" K")
            );
        }
    }

    // 仿真设置
    {
        SimulationConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/simulation.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && SimulationConfigManager::load(project.projectPath, config);

        CheckStatus status = CheckStatus::Pass;
        if (!exists) {
            status = CheckStatus::Missing;
        } else if (!valid) {
            status = CheckStatus::Invalid;
        }

        if (status != CheckStatus::Pass) {
            allPassed = false;
        }

        QTreeWidgetItem *group = addGroupItem(
            treeWidget,
            QStringLiteral("仿真设置"),
            status
        );

        if (status == CheckStatus::Pass) {
            addChildValue(
                group,
                QStringLiteral("时间长度"),
                formatNumber(config.timeLength) + QStringLiteral(" s")
            );
        }
    }

    if (allPassed) {
        summaryLabel->setText(
            QStringLiteral("✓ 参数检查通过，可以生成 Abaqus 文件。")
        );
        summaryLabel->setStyleSheet(R"(
            QLabel {
                background-color: #f6ffed;
                border: 1px solid #b7eb8f;
                border-radius: 5px;
                color: #389e0d;
                font-family: 'Microsoft YaHei';
                font-size: 16px;
                font-weight: bold;
                padding: 10px 15px;
            }
        )");
    } else {
        summaryLabel->setText(
            QStringLiteral("参数检查未通过，请检查未保存或无效的参数模块。")
        );
        summaryLabel->setStyleSheet(R"(
            QLabel {
                background-color: #fff2f0;
                border: 1px solid #ffccc7;
                border-radius: 5px;
                color: #cf1322;
                font-family: 'Microsoft YaHei';
                font-size: 16px;
                font-weight: bold;
                padding: 10px 15px;
            }
        )");
    }
}
