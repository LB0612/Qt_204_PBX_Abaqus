#ifndef PARAMETERCHECKWIDGET_H
#define PARAMETERCHECKWIDGET_H

#include "BaseParamWidget.h"
#include "ProjectManager.h"

class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;

class ParameterCheckWidget : public BaseParamWidget
{
    Q_OBJECT

public:
    explicit ParameterCheckWidget(QWidget *parent = nullptr);

    void refresh(const ProjectConfig &project);

private:
    void setupUi();
    QGroupBox *createSectionBox(const QString &title);
    QHBoxLayout *createInfoRow(const QString &label, const QString &value);
    void clearSections();
    void addWarningLabel(QVBoxLayout *layout, const QString &message);

    QVBoxLayout *sectionsLayout = nullptr;
};

#endif
