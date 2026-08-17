#ifndef PROJECTINFOWIDGET_H
#define PROJECTINFOWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "ProjectManager.h"
#include "BaseParamWidget.h" // 继承基类

class ProjectInfoWidget : public BaseParamWidget
{
    Q_OBJECT
public:
    explicit ProjectInfoWidget(QWidget *parent = nullptr);
    void setProjectData(const ProjectConfig &config);
    void getProjectData(ProjectConfig &config) const;

private:
    void setupUi();
    QLineEdit *nameEdit, *typeEdit, *dateEdit, *pathEdit;
};

#endif
