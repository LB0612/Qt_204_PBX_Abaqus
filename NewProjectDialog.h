#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);

    QString getProjectName() const;
    QString getProjectPath() const;
    int getProcessType() const;
    QString getTypeName() const;
    
    // 覆盖 QDialog 的 accept() 方法
    void accept() override;

private slots:
    void onBrowsePath();

private:
    void setupUi();
    void applyStyles();

    QLineEdit *nameEdit;
    QLineEdit *pathEdit;
    QComboBox *typeCombo;
    QPushButton *browseBtn;
    QPushButton *okBtn;
    QPushButton *cancelBtn;
    QLabel *titleLabel;
};

#endif
