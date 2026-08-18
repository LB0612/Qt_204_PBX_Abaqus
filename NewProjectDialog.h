#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);

    QString getProjectName() const;
    QString getProjectPath() const;

    void accept() override;

private slots:
    void onBrowsePath();

private:
    void setupUi();
    void applyStyles();
    void updateOkButtonState();

    QLineEdit *nameEdit;
    QLineEdit *pathEdit;
    QPushButton *browseBtn;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
    QLabel *titleLabel;
};

#endif
