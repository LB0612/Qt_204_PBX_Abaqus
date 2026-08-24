#ifndef OPENPROJECTDIALOG_H
#define OPENPROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

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
    QTableWidget *projectTable;
    QPushButton *openBtn;
    QPushButton *cancelBtn;
};

#endif
