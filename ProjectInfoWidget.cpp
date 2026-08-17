#include "ProjectInfoWidget.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMessageBox>
#include <QScrollArea>

ProjectInfoWidget::ProjectInfoWidget(QWidget *parent) : BaseParamWidget(parent)
{
    setupUi();
    applyCommonStyles();
}

void ProjectInfoWidget::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout(this);
    setupHeader("工程基本信息");

    QScrollArea *scrollArea = createScrollArea(this);
    QWidget *content = new QWidget();
    content->setObjectName("ScrollContent");

    // 使用统一的布局管理器
    QVBoxLayout *scrollLayout = createScrollContentLayout(content);

    // 初始化控件
    nameEdit = createReadOnlyEdit();
    typeEdit = createReadOnlyEdit();
    dateEdit = createReadOnlyEdit();
    pathEdit = createReadOnlyEdit();

    // 【升级】使用 addParamRow，这就不用那堆旧代码了
    // addParamRow 会自动完成参数注册，无需手动调用 registerParam
    addParamRow(scrollLayout, "工程名称", nameEdit);
    addParamRow(scrollLayout, "工艺类型", typeEdit);
    addParamRow(scrollLayout, "创建时间", dateEdit);
    addParamRow(scrollLayout, "存储路径", pathEdit);

    // 按钮
    addSaveButton(scrollLayout, "打开所在文件夹", [this](){
        QString path = pathEdit->text();
        if (!path.isEmpty() && QDir(path).exists())
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        else {
            QMessageBox msgBox(QMessageBox::Warning, "错误", "文件夹不存在", QMessageBox::Ok, this);
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.exec();
        }
    });

    scrollLayout->addStretch(); // 顶上去

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

void ProjectInfoWidget::setProjectData(const ProjectConfig &config)
{
    nameEdit->setText(config.projectName);
    
    // 优化：使用 switch 结构，或者调用 ProjectManager 的静态转换函数（如果有）
    QString typeName;
    switch(config.processType) {
        case 0: 
            typeName = "捏合混匀工艺"; 
            break;
        case 1: 
            typeName = "挤压造粒工艺"; 
            break;
        case 2: 
            typeName = "压制工艺"; 
            break;
        default: 
            typeName = "未知工艺"; 
            break;
    }
    typeEdit->setText(typeName);

    dateEdit->setText(config.createdDate);
    pathEdit->setText(config.projectPath);
}

void ProjectInfoWidget::getProjectData(ProjectConfig &config) const
{
    config.projectName = nameEdit->text();
}
