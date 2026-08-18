#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    static QString getAbaqusPath();

private slots:
    void onBrowseExe();
    void onSave();

private:
    void setupUi();
    void loadSettings();

    QLineEdit *m_exePathEdit;
    QPushButton *m_browseExeBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif
