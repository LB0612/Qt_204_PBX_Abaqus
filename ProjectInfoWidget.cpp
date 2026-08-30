#include "ProjectInfoWidget.h"

#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QScrollArea>
#include <QUrl>

ProjectInfoWidget::ProjectInfoWidget(QWidget *parent)
    : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void ProjectInfoWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader(QStringLiteral("工程信息"));

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("ScrollContent"));

    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    nameEdit = createReadOnlyEdit();
    typeEdit = createReadOnlyEdit();
    dateEdit = createReadOnlyEdit();
    pathEdit = createReadOnlyEdit();

    addParamRow(scrollLayout, QStringLiteral("工程名称"), nameEdit);
    addParamRow(scrollLayout, QStringLiteral("工程类型"), typeEdit);
    addParamRow(scrollLayout, QStringLiteral("创建时间"), dateEdit);
    addParamRow(scrollLayout, QStringLiteral("工程路径"), pathEdit);

    addSaveButton(scrollLayout, QStringLiteral("打开所在文件夹"), [this]() {
        const QString path = pathEdit->text();
        if (!path.isEmpty() && QDir(path).exists()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            return;
        }

        QMessageBox msgBox(QMessageBox::Warning, QStringLiteral("错误"), QStringLiteral("文件夹不存在"), QMessageBox::Ok, this);
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.exec();
    });

    scrollLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void ProjectInfoWidget::setProjectData(const ProjectConfig &config)
{
    nameEdit->setText(config.projectName);

    if (config.projectType == QStringLiteral("PBX_CASTING_CURING")) {
        typeEdit->setText(QStringLiteral("浇注XX固化过程仿真分析与三维参数重构分析软件"));
    } else {
        typeEdit->setText(QStringLiteral("未知工程类型"));
    }

    dateEdit->setText(config.createdDate);
    pathEdit->setText(config.projectPath);
}

void ProjectInfoWidget::getProjectData(ProjectConfig &config) const
{
    config.projectName = nameEdit->text();
}
