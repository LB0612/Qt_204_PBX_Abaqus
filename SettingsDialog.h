#ifndef SETTINGSDIALOG_H 
#define SETTINGSDIALOG_H 

#include <QDialog> 
#include <QLineEdit> 
#include <QPushButton> 
#include <QLabel> 

class SettingsDialog : public QDialog 
{ 
    Q_OBJECT 

public: 
    explicit SettingsDialog(QWidget *parent = nullptr); 
    // 静态工具函数：供 MainWindow 获取已保存的路径 
    static QString getPolyflowPath(); 

private slots: 
    void onAutoScan();     // 简化后的智能扫描 
    void onBrowseExe();    // 直接浏览文件 
    void onSave();         // 校验并保存 

private: 
    void setupUi(); 
    void loadSettings(); 

    QLineEdit *m_exePathEdit; 
    QLabel    *m_statusLabel; 
    QPushButton *m_autoScanBtn; 
    QPushButton *m_browseExeBtn; 
    QPushButton *m_saveBtn; 
    QPushButton *m_cancelBtn; 
}; 

#endif // SETTINGSDIALOG_H