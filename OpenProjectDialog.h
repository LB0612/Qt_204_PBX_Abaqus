#ifndef OPENPROJECTDIALOG_H
#define OPENPROJECTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "ProjectManager.h"

class OpenProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenProjectDialog(QWidget *parent = nullptr);
    QStringList getSelectedPaths() const;

private:
    void setupUi();
    void applyStyles();
    void scanProjects();

    QLineEdit *rootPathEdit;
    QPushButton *browseBtn;
    QComboBox *typeFilterCombo;
    QTableWidget *projectTable;
    QPushButton *openBtn;
    QPushButton *cancelBtn;
    QLabel *titleLabel;

    QString selectedProjectPath;
    QList<ProjectConfig> allProjects;
};

#endif
