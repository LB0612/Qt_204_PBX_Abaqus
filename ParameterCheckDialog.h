#ifndef PARAMETERCHECKDIALOG_H
#define PARAMETERCHECKDIALOG_H

#include <QDialog>

#include "ProjectManager.h"

class QTreeWidget;
class QLabel;

class ParameterCheckDialog : public QDialog
{
public:
    explicit ParameterCheckDialog(
        const ProjectConfig &project,
        QWidget *parent = nullptr
    );

private:
    void setupUi(const ProjectConfig &project);

    QTreeWidget *treeWidget = nullptr;
    QLabel *summaryLabel = nullptr;
};

#endif
