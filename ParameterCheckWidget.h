#ifndef PARAMETERCHECKWIDGET_H
#define PARAMETERCHECKWIDGET_H

#include "BaseParamWidget.h"
#include "ProjectManager.h"

class QTreeWidget;
class QLabel;

class ParameterCheckWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit ParameterCheckWidget(QWidget *parent = nullptr);

    void refresh(const ProjectConfig &project);

private:
    void setupUi();

    QTreeWidget *treeWidget = nullptr;
    QLabel *summaryLabel = nullptr;
};

#endif
