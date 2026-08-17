#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QTreeWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QResizeEvent>
#include <QFileDialog>
#include <QStackedWidget>
#include <QMap>
#include <QFileSystemWatcher>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QProgressBar>

// 包含必要的头文件
#include "ProjectManager.h"
#include "ProjectInfoWidget.h"
#include "BaseParamWidget.h"
#include "SettingsDialog.h"
#include "SimulationManager.h"

//// 前置声明其他类
class KneadingExplosiveWidget;
class ExtrusionExplosiveWidget;
class KneadingBladeWidget;
class ExtrusionScrewWidget;
class KneadingBoundaryWidget;
class ExtrusionBoundaryWidget;
class KneadingSimulationWidget;
class ExtrusionSimulationWidget;
class NewProjectDialog;
class OpenProjectDialog;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupWidgetCallbacks();
    void createPureStyleToolBar();
    void createTreeWidget();
    void updateTreeStructure(const QString &name, int type, const QString &path);
    void selectTreeItem(const QString &itemName);
    void startWatchingProject();
    void stopWatchingProject();
    void loadAllProjectData(const ProjectConfig &config);
    void setupLogOutput(); // 设置日志输出界面
    void updateSimulationActionStates(); // 更新仿真相关按钮状态

    ProjectConfig currentProject;
    bool isProjectLoaded = false;

    QWidget *centralWidget;
    QHBoxLayout *mainLayout;
    QToolBar *toolBar;
    QTreeWidget *treeWidget;
    QStackedWidget *stackedWidget;
    QLabel *titleLabel;
    ProjectInfoWidget *infoWidget;

    // 子页面指针
    KneadingExplosiveWidget *kneadingExplosiveWidget;
    ExtrusionExplosiveWidget *extrusionExplosiveWidget;
    KneadingBladeWidget *kneadingBladeWidget;
    ExtrusionScrewWidget *extrusionScrewWidget;
    KneadingBoundaryWidget *kneadingBoundaryWidget;
    ExtrusionBoundaryWidget *extrusionBoundaryWidget;
    KneadingSimulationWidget *kneadingSimulationWidget;
    ExtrusionSimulationWidget *extrusionSimulationWidget;

    QFileSystemWatcher *fileWatcher;
    QTimer *debounceTimer;

    // 仿真监控控件
    QWidget *simulationMonitorWidget = nullptr;
    QTextEdit *logOutput = nullptr;
    QProgressBar *progressBar = nullptr;
    QLabel *statusLabel = nullptr;

    QAction *startAction = nullptr;
    QAction *generateAction = nullptr; // 【新增】保存生成文件的动作指针

    

    QList<QAction*> projectDependentActions;

    void updateUIStates();

    // 辅助函数：显示居中的消息框
    QMessageBox::StandardButton showCenteredMessageBox(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    // =========== 【第一步：新增辅助函数声明】 开始 ===========
    
    // 捏合工艺专用处理函数
    bool generateKneadingFiles();      // 生成捏合仿真文件
    void startKneadingSimulation();    // 启动捏合仿真

    // 挤压工艺专用处理函数
    bool generateExtrusionFiles();     // 生成挤压仿真文件
    void startExtrusionSimulation();   // 启动挤压仿真

    // =========== 【第一步：新增辅助函数声明】 结束 ===========

private slots:
    void newProject();
    void openProject();
    void saveProject(bool silent = false);
    void exitProject();

    void projectInfo();
    void explosiveParams();
    void bladeParams();
    void boundaryConditions();
    void simulationSettings();

    void checkParams();
    void generateFiles();
    void startSimulation();
    void generateReport();

    void settings();
    void help();

    void terminateSimulation();

    // 响应SimulationManager信号的槽函数
    void onSimulationLogReceived(QString projectPath, QString message);
    void onSimulationProgress(QString projectPath, int percent, QString status);
    void onSimulationFinished(QString projectPath, int exitCode);

    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onProjectDirectoryChanged();
};

#endif // MAINWINDOW_H
