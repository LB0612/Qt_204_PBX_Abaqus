#include "ParameterCheckDialog.h"

#include "BoundaryConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "SimulationConfigManager.h"
#include "StructureConfigManager.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
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

ParameterCheckDialog::ParameterCheckDialog(
    const ProjectConfig &project,
    QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("参数检查"));
    setModal(true);
    resize(720, 560);
    setupUi(project);
}

void ParameterCheckDialog::setupUi(const ProjectConfig &project)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    treeWidget = new QTreeWidget(this);
    treeWidget->setColumnCount(3);
    treeWidget->setHeaderLabels({
        QStringLiteral("检查项"),
        QStringLiteral("当前值"),
        QStringLiteral("状态")
    });
    treeWidget->setRootIsDecorated(true);
    treeWidget->setAlternatingRowColors(true);
    treeWidget->setUniformRowHeights(true);
    treeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
    treeWidget->setFocusPolicy(Qt::NoFocus);

    QHeaderView *header = treeWidget->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

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

    mainLayout->addWidget(treeWidget, 1);

    summaryLabel = new QLabel(this);
    summaryLabel->setWordWrap(true);
    summaryLabel->setStyleSheet(QStringLiteral("font-size: 14px; padding: 4px 2px;"));
    if (allPassed) {
        summaryLabel->setText(
            QStringLiteral("参数检查通过，可以生成 Abaqus 文件。")
        );
        summaryLabel->setStyleSheet(
            QStringLiteral("font-size: 14px; padding: 4px 2px; color: #389e0d;")
        );
    } else {
        summaryLabel->setText(
            QStringLiteral("参数检查未通过，请检查未保存或无效的参数模块。")
        );
        summaryLabel->setStyleSheet(
            QStringLiteral("font-size: 14px; padding: 4px 2px; color: #cf1322;")
        );
    }
    mainLayout->addWidget(summaryLabel);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Close,
        this
    );
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);
}
