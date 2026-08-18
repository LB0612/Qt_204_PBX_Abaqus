#ifndef PROJECTINFOWIDGET_H
#define PROJECTINFOWIDGET_H

#include "BaseParamWidget.h"
#include "ProjectManager.h"

class ProjectInfoWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit ProjectInfoWidget(QWidget *parent = nullptr);

    void setProjectData(const ProjectConfig &config);
    void getProjectData(ProjectConfig &config) const;

private:
    void setupUi();

    QLineEdit *nameEdit;
    QLineEdit *typeEdit;
    QLineEdit *dateEdit;
    QLineEdit *pathEdit;
    QLineEdit *versionEdit;
};

#endif
