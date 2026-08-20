#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAction>
#include <QToolBar>
#include <QTreeWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QMessageBox>
#include <QProcess>

#include "ProjectManager.h"
#include "ProjectInfoWidget.h"
#include "StructureParamWidget.h"
#include "ExplosiveParamWidget.h"
#include "MoldParamWidget.h"
#include "BoundaryParamWidget.h"
#include "SimulationParamWidget.h"
#include "ParameterCheckWidget.h"
#include "SimulationMonitorWidget.h"

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
    void createPureStyleToolBar();
    void createTreeWidget();
    void updateTreeStructure(const QString &name, const QString &path);
    void selectTreeItem(const QString &itemName);
    void startWatchingProject();
    void stopWatchingProject();
    void loadProjectToUi();
    void updateUIStates();

    QMessageBox::StandardButton showCenteredMessageBox(
        QWidget *parent,
        QMessageBox::Icon icon,
        const QString &title,
        const QString &text,
        QMessageBox::StandardButtons buttons = QMessageBox::Ok,
        QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    ProjectConfig currentProject;
    bool isProjectLoaded = false;

    QWidget *centralWidget;
    QHBoxLayout *mainLayout;
    QToolBar *toolBar;
    QTreeWidget *treeWidget;
    QStackedWidget *stackedWidget;
    QLabel *titleLabel;
    ProjectInfoWidget *infoWidget;
    StructureParamWidget *structureWidget;
    ExplosiveParamWidget *explosiveWidget;
    MoldParamWidget *moldWidget;
    BoundaryParamWidget *boundaryWidget;
    SimulationParamWidget *simulationWidget;
    ParameterCheckWidget *parameterCheckWidget;
    SimulationMonitorWidget *simulationMonitorWidget;

    QFileSystemWatcher *fileWatcher;
    QTimer *debounceTimer;
    QProcess *abaqusProcess = nullptr;

    QList<QAction *> projectDependentActions;

private slots:
    void newProject();
    void openProject();
    void saveProject(bool silent = false);
    void exitProject();
    void projectInfo();
    void structureParams();
    void saveStructureParams();
    void explosiveParams();
    void saveExplosiveParams();
    void moldParams();
    void saveMoldParams();
    void boundaryParams();
    void saveBoundaryParams();
    void simulationParams();
    void saveSimulationParams();
    void checkParams();
    void generateFiles();
    void startSimulation();
    void settings();
    void help();

    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onProjectDirectoryChanged();
};

#endif
