#include "ParameterCheckWidget.h"

#include "BoundaryConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "SimulationConfigManager.h"
#include "StructureConfigManager.h"

#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QString formatNumber(double value)
{
    return QString::number(value, 'g', 15);
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
    sectionsLayout = createScrollContentLayout(content);
    sectionsLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QGroupBox *ParameterCheckWidget::createSectionBox(const QString &title)
{
    QGroupBox *box = new QGroupBox(title);
    box->setStyleSheet(
        "QGroupBox {"
        "font-family: 'Microsoft YaHei';"
        "font-weight: bold;"
        "font-size: 18px;"
        "border: 1px solid #ccc;"
        "border-radius: 6px;"
        "margin-top: 10px;"
        "padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "left: 10px;"
        "padding: 0 5px;"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setSpacing(2);
    layout->setContentsMargins(10, 15, 10, 10);
    return box;
}

QHBoxLayout *ParameterCheckWidget::createInfoRow(
    const QString &label,
    const QString &value)
{
    QHBoxLayout *row = new QHBoxLayout();

    QLabel *lblKey = new QLabel(label);
    lblKey->setFixedWidth(180);
    lblKey->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblKey->setMinimumHeight(38);
    lblKey->setStyleSheet(
        "font-family: 'Microsoft YaHei';"
        "color: #000;"
        "font-weight: bold;"
        "font-size: 16px;"
    );

    QLineEdit *valueEdit = new QLineEdit(value);
    valueEdit->setReadOnly(true);
    valueEdit->setMinimumHeight(38);
    valueEdit->setStyleSheet(
        "border: none;"
        "background: transparent;"
        "color: #333;"
        "margin-left: 5px;"
        "font-family: 'Microsoft YaHei';"
        "font-size: 16px;"
        "font-weight: bold;"
    );

    row->addWidget(lblKey);
    row->addWidget(valueEdit);
    row->setContentsMargins(0, 0, 0, 0);
    return row;
}

void ParameterCheckWidget::clearSections()
{
    if (!sectionsLayout) {
        return;
    }

    while (sectionsLayout->count() > 0) {
        QLayoutItem *item = sectionsLayout->takeAt(0);
        if (!item) {
            continue;
        }
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void ParameterCheckWidget::addWarningLabel(
    QVBoxLayout *layout,
    const QString &message)
{
    QLabel *warningLabel = new QLabel(message);
    warningLabel->setStyleSheet(
        "font-family: 'Microsoft YaHei';"
        "font-size: 16px;"
        "color: #cf1322;"
    );
    layout->addWidget(warningLabel);
}

void ParameterCheckWidget::refresh(const ProjectConfig &project)
{
    clearSections();

    // 1. 工程信息
    {
        QGroupBox *box = createSectionBox(QStringLiteral("1. 工程信息"));
        QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());

        ProjectConfig loaded;
        const QString projectJsonPath =
            QDir(project.projectPath).filePath(QStringLiteral("project.json"));
        const bool exists = QFileInfo::exists(projectJsonPath);
        const bool valid =
            exists
            && ProjectManager::loadProject(project.projectPath, loaded)
            && !loaded.projectName.trimmed().isEmpty();

        if (!exists) {
            addWarningLabel(boxLayout, QStringLiteral("工程信息文件不存在"));
        } else if (!valid) {
            addWarningLabel(boxLayout, QStringLiteral("工程信息文件无效"));
        } else {
            boxLayout->addLayout(
                createInfoRow(QStringLiteral("工程名称："), loaded.projectName)
            );
        }

        sectionsLayout->addWidget(box);
    }

    // 2. 炸药参数
    {
        QGroupBox *box = createSectionBox(QStringLiteral("2. 炸药参数"));
        QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());

        ExplosiveConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/explosive.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && ExplosiveConfigManager::load(project.projectPath, config);

        if (!exists) {
            addWarningLabel(boxLayout, QStringLiteral("炸药参数未保存"));
        } else if (!valid) {
            addWarningLabel(boxLayout, QStringLiteral("炸药参数配置文件无效"));
        } else {
            boxLayout->addLayout(createInfoRow(QStringLiteral("炸药密度："), formatNumber(config.density)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("固化初始杨氏模量："), formatNumber(config.initialElasticModulus)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("固化初始泊松比："), formatNumber(config.initialPoissonRatio)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("固化结束杨氏模量："), formatNumber(config.finalElasticModulus)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("固化结束泊松比："), formatNumber(config.finalPoissonRatio)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("炸药传导率："), formatNumber(config.thermalConductivity)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("炸药屈服应力："), formatNumber(config.yieldStress)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("炸药比热："), formatNumber(config.specificHeat)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("炸药膨胀系数："), formatNumber(config.expansionCoefficient)));
        }

        sectionsLayout->addWidget(box);
    }

    // 3. 结构参数
    {
        QGroupBox *box = createSectionBox(QStringLiteral("3. 结构参数"));
        QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());

        StructureConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/structure.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && StructureConfigManager::load(project.projectPath, config);

        if (!exists) {
            addWarningLabel(boxLayout, QStringLiteral("结构参数未保存"));
        } else if (!valid) {
            addWarningLabel(boxLayout, QStringLiteral("结构参数配置文件无效"));
        } else {
            boxLayout->addLayout(createInfoRow(QStringLiteral("药柱半径："), formatNumber(config.chargeRadius)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("药柱高度："), formatNumber(config.chargeHeight)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("外壳厚度："), formatNumber(config.shellThickness)));
        }

        sectionsLayout->addWidget(box);
    }

    // 4. 模具参数
    {
        QGroupBox *box = createSectionBox(QStringLiteral("4. 模具参数"));
        QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());

        MoldConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/mold.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && MoldConfigManager::load(project.projectPath, config);

        if (!exists) {
            addWarningLabel(boxLayout, QStringLiteral("模具参数未保存"));
        } else if (!valid) {
            addWarningLabel(boxLayout, QStringLiteral("模具参数配置文件无效"));
        } else {
            boxLayout->addLayout(createInfoRow(QStringLiteral("模具密度："), formatNumber(config.density)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("模具弹性模量："), formatNumber(config.elasticModulus)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("模具泊松比："), formatNumber(config.poissonRatio)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("模具热导率："), formatNumber(config.thermalConductivity)));
            boxLayout->addLayout(createInfoRow(QStringLiteral("模具比热容："), formatNumber(config.specificHeat)));
        }

        sectionsLayout->addWidget(box);
    }

    // 5. 边界条件
    {
        QGroupBox *box = createSectionBox(QStringLiteral("5. 边界条件"));
        QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());

        BoundaryConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/boundary.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && BoundaryConfigManager::load(project.projectPath, config);

        if (!exists) {
            addWarningLabel(boxLayout, QStringLiteral("边界条件未保存"));
        } else if (!valid) {
            addWarningLabel(boxLayout, QStringLiteral("边界条件配置文件无效"));
        } else {
            boxLayout->addLayout(
                createInfoRow(
                    QStringLiteral("环境温度："),
                    formatNumber(config.ambientTemperature)
                )
            );
        }

        sectionsLayout->addWidget(box);
    }

    // 6. 仿真设置
    {
        QGroupBox *box = createSectionBox(QStringLiteral("6. 仿真设置"));
        QVBoxLayout *boxLayout = qobject_cast<QVBoxLayout *>(box->layout());

        SimulationConfig config;
        const QString filePath =
            QDir(project.projectPath)
                .filePath(QStringLiteral("config/simulation.json"));
        const bool exists = QFileInfo::exists(filePath);
        const bool valid =
            exists && SimulationConfigManager::load(project.projectPath, config);

        if (!exists) {
            addWarningLabel(boxLayout, QStringLiteral("仿真设置未保存"));
        } else if (!valid) {
            addWarningLabel(boxLayout, QStringLiteral("仿真设置配置文件无效"));
        } else {
            boxLayout->addLayout(
                createInfoRow(
                    QStringLiteral("时间长度："),
                    formatNumber(config.timeLength)
                )
            );
        }

        sectionsLayout->addWidget(box);
    }

    sectionsLayout->addStretch();
}
