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

#include "ProjectManager.h"
#include "SimulationManager.h"
#include "ProjectInfoWidget.h"
#include "StructureParamWidget.h"
#include "ExplosiveParamWidget.h"
#include "MoldParamWidget.h"
#include "BoundaryParamWidget.h"
#include "SimulationParamWidget.h"
#include "ParameterCheckWidget.h"
#include "SimulationMonitorWidget.h"
#include "SimulationPrepareWidget.h"

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
    void connectSimulationManager();

    bool ensureSimulationIdle(const QString &operation);
    bool ensureParameterWritable(const QString &parameterName);

    void setParameterPagesReadOnly(bool readOnly);
    void reloadParameterPagesFromSavedConfig();

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
    SimulationPrepareWidget *simulationPrepareWidget;

    SimulationManager *simulationManager = nullptr;

    QFileSystemWatcher *fileWatcher;
    QTimer *debounceTimer;

    QList<QAction *> projectDependentActions;
    QList<QAction *> simulationLockedActions;

    QAction *stopSimulationAction = nullptr;

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
    void showSimulationPreparePage();
    void showPreviousSimulationLogs();
    void startSimulation();
    void stopSimulation();
    void settings();
    void help();

    void onSimulationStateChanged(SimulationState state);
    void onForceKillRequested();
    void onSimulationErrorOccurred(const QString &title, const QString &text);
    void onSimulationFinished();
    void onSimulationFailed(const QString &error);
    void onSimulationStopped();

    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onProjectDirectoryChanged();
};

#endif
