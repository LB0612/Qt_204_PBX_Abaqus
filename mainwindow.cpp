#include "mainwindow.h"

#include "NewProjectDialog.h"
#include "OpenProjectDialog.h"
#include "SettingsDialog.h"
#include "StructureConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "BoundaryConfigManager.h"
#include "SimulationConfigManager.h"
#include "ParameterCheckWidget.h"
#include "SimulationMonitorWidget.h"
#include "AbaqusFileGenerator.h"

#include <QBrush>
#include <QCloseEvent>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGridLayout>
#include <QIcon>
#include <QProcess>
#include <QSet>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {

const int ROLE_NODE_TYPE = Qt::UserRole + 1;

const QString NODE_ROOT =
    QStringLiteral("ROOT");

const QString NODE_PROJECT =
    QStringLiteral("PROJECT");

const QString NODE_PROJECT_INFO =
    QStringLiteral("PROJECT_INFO");

const QString NODE_STRUCTURE =
    QStringLiteral("STRUCTURE");

const QString NODE_EXPLOSIVE =
    QStringLiteral("EXPLOSIVE");

const QString NODE_MOLD =
    QStringLiteral("MOLD");

const QString NODE_BOUNDARY =
    QStringLiteral("BOUNDARY");

const QString NODE_SIMULATION =
    QStringLiteral("SIMULATION");

const QString NODE_PARAMETER_CHECK =
    QStringLiteral("PARAMETER_CHECK");

const QString NODE_START_SIMULATION =
    QStringLiteral("START_SIMULATION");

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("浇注XX固化仿真分析工程"));
    setMinimumSize(1300, 800);
    resize(1300, 800);
    setupUi();

    setStyleSheet(R"(
    QMainWindow {
        border-image: url(:/new/prefix1/toolbar_picture/back.png) 0 0 0 0 stretch stretch;
    }
    )");

    fileWatcher = new QFileSystemWatcher(this);
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(100);

    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() { debounceTimer->start(); });
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this]() { debounceTimer->start(); });
    connect(debounceTimer, &QTimer::timeout, this, &MainWindow::onProjectDirectoryChanged);

    updateUIStates();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    createPureStyleToolBar();

    centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("centralWidget"));
    centralWidget->setStyleSheet(QStringLiteral("QWidget#centralWidget { background: transparent; }"));

    mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *leftWidget = new QWidget(centralWidget);
    leftWidget->setFixedWidth(300);
    leftWidget->setStyleSheet(QStringLiteral("background-color: #ffffff; border-right: 1px solid #e6e6e6;"));

    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    createTreeWidget();
    leftLayout->addWidget(treeWidget);

    QWidget *rightWidget = new QWidget(centralWidget);
    rightWidget->setStyleSheet(QStringLiteral("background: transparent;"));

    QGridLayout *rightLayout = new QGridLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    stackedWidget = new QStackedWidget(rightWidget);
    stackedWidget->setAttribute(Qt::WA_TranslucentBackground);

    titleLabel = new QLabel(QStringLiteral("浇注XX固化仿真分析工程"), stackedWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(QStringLiteral("font-family: 'Microsoft YaHei'; font-size: 60px; color: #333; font-weight: bold; background: transparent; padding: 20px;"));
    stackedWidget->addWidget(titleLabel);

    infoWidget = new ProjectInfoWidget(stackedWidget);
    stackedWidget->addWidget(infoWidget);

    structureWidget = new StructureParamWidget(stackedWidget);
    stackedWidget->addWidget(structureWidget);

    explosiveWidget = new ExplosiveParamWidget(stackedWidget);
    stackedWidget->addWidget(explosiveWidget);

    moldWidget = new MoldParamWidget(stackedWidget);
    stackedWidget->addWidget(moldWidget);

    boundaryWidget = new BoundaryParamWidget(stackedWidget);
    stackedWidget->addWidget(boundaryWidget);

    simulationWidget = new SimulationParamWidget(stackedWidget);
    stackedWidget->addWidget(simulationWidget);

    parameterCheckWidget = new ParameterCheckWidget(stackedWidget);
    stackedWidget->addWidget(parameterCheckWidget);

    simulationMonitorWidget = new SimulationMonitorWidget(stackedWidget);
    stackedWidget->addWidget(simulationMonitorWidget);

    connect(infoWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(structureWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(structureWidget, &StructureParamWidget::saveRequested,
            this, &MainWindow::saveStructureParams);

    connect(explosiveWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(explosiveWidget, &ExplosiveParamWidget::saveRequested,
            this, &MainWindow::saveExplosiveParams);

    connect(moldWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(moldWidget, &MoldParamWidget::saveRequested,
            this, &MainWindow::saveMoldParams);

    connect(boundaryWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(boundaryWidget, &BoundaryParamWidget::saveRequested,
            this, &MainWindow::saveBoundaryParams);

    connect(simulationWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(simulationWidget, &SimulationParamWidget::saveRequested,
            this, &MainWindow::saveSimulationParams);

    connect(parameterCheckWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    rightLayout->addWidget(stackedWidget, 0, 0, 1, 1);

    mainLayout->addWidget(leftWidget);
    mainLayout->addWidget(rightWidget);
    setCentralWidget(centralWidget);
}

void MainWindow::createPureStyleToolBar()
{
    QToolBar *oldBar = findChild<QToolBar *>();
    if (oldBar) {
        delete oldBar;
    }

    toolBar = addToolBar(tr("工具栏"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(40, 40));
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setStyleSheet(R"(
    QToolBar {
        background-color: rgba(255, 255, 255, 0.95);
        border-bottom: 1px solid #e0e0e0;
        spacing: 1px;
        padding: 3px;
    }
    QToolButton {
        background: transparent;
        border: none;
        border-radius: 6px;
        color: #000000;
        font-family: 'SimSun', '宋体';
        font-weight: 800;
        font-size: 16px;
        padding: 2px 3px;
        min-width: 60px;
    }
    QToolButton:hover {
        background-color: #f0f7ff;
        border: 1px solid #cce5ff;
    }
    QToolButton:pressed {
        background-color: #e6f7ff;
    }
    )");

    auto addBtn = [&](const QString &txt, const QString &icon, void (MainWindow::*slot)(), bool needsProject = true) {
        QAction *act = new QAction(QIcon(icon), txt, this);
        connect(act, &QAction::triggered, this, slot);
        toolBar->addAction(act);
        if (needsProject) {
            projectDependentActions.append(act);
        }
    };

    auto addPlaceholderBtn = [&](const QString &txt, const QString &icon) {
        QAction *act = new QAction(QIcon(icon), txt, this);
        toolBar->addAction(act);
        projectDependentActions.append(act);
    };

    addBtn(QStringLiteral("新建工程"), QStringLiteral(":/new/prefix1/toolbar_picture/create.png"), &MainWindow::newProject, false);
    addBtn(QStringLiteral("打开工程"), QStringLiteral(":/new/prefix1/toolbar_picture/open.png"), &MainWindow::openProject, false);

    QAction *saveAct = new QAction(QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/save.png")), QStringLiteral("保存工程"), this);
    connect(saveAct, &QAction::triggered, this, [this]() { saveProject(false); });
    toolBar->addAction(saveAct);
    projectDependentActions.append(saveAct);

    addBtn(QStringLiteral("关闭工程"), QStringLiteral(":/new/prefix1/toolbar_picture/close.png"), &MainWindow::exitProject);

    toolBar->addSeparator();

    addBtn(QStringLiteral("工程信息"), QStringLiteral(":/new/prefix1/toolbar_picture/information.png"), &MainWindow::projectInfo);
    addBtn(QStringLiteral("炸药参数"), QStringLiteral(":/new/prefix1/toolbar_picture/cailiaocanshu.png"), &MainWindow::explosiveParams);
    addBtn(QStringLiteral("结构参数"), QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png"), &MainWindow::structureParams);
    addBtn(QStringLiteral("模具参数"), QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png"), &MainWindow::moldParams);
    addBtn(QStringLiteral("边界条件"), QStringLiteral(":/new/prefix1/toolbar_picture/fangzhenshezhi.png"), &MainWindow::boundaryParams);
    addBtn(QStringLiteral("仿真设置"), QStringLiteral(":/new/prefix1/toolbar_picture/fangzhenshezhi.png"), &MainWindow::simulationParams);

    toolBar->addSeparator();

    addBtn(QStringLiteral("参数检查"), QStringLiteral(":/new/prefix1/toolbar_picture/check.png"), &MainWindow::checkParams);
    addBtn(QStringLiteral("生成文件"), QStringLiteral(":/new/prefix1/toolbar_picture/file.png"), &MainWindow::generateFiles);
    addBtn(QStringLiteral("开始仿真"), QStringLiteral(":/new/prefix1/toolbar_picture/start.png"), &MainWindow::startSimulation);
    addPlaceholderBtn(QStringLiteral("生成报告"), QStringLiteral(":/new/prefix1/toolbar_picture/report.png"));

    toolBar->addSeparator();

    addBtn(QStringLiteral("系统设置"), QStringLiteral(":/new/prefix1/toolbar_picture/setup.png"), &MainWindow::settings, false);
    addBtn(QStringLiteral("帮助文档"), QStringLiteral(":/new/prefix1/toolbar_picture/help.png"), &MainWindow::help, false);

    QAction *closeAppAct = new QAction(QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/closeall.png")), QStringLiteral("关闭"), this);
    if (closeAppAct->icon().isNull()) {
        closeAppAct->setIcon(QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/close.png")));
    }
    connect(closeAppAct, &QAction::triggered, this, &MainWindow::close);
    toolBar->addAction(closeAppAct);
}

void MainWindow::createTreeWidget()
{
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderHidden(true);
    treeWidget->setColumnCount(1);
    treeWidget->setIndentation(20);
    treeWidget->setStyleSheet(
        "QTreeWidget { background: transparent; border: none; font-size: 14px; outline: none; }"
        "QTreeWidget::item { height: 32px; padding-left: 5px; border-bottom: 1px solid transparent; }"
        "QTreeWidget::item:hover { background-color: #f0f0f0; }"
        "QTreeWidget::item:selected { background-color: #e6f7ff; color: #1890ff; border-left: 3px solid #1890ff; }"
    );

    connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &MainWindow::onTreeItemDoubleClicked);
    connect(treeWidget, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);

    QTreeWidgetItem *rootItem = new QTreeWidgetItem(treeWidget);
    rootItem->setText(0, QStringLiteral("浇注XX固化仿真分析工程"));
    rootItem->setData(0, Qt::UserRole, QStringLiteral("CATEGORY_ROOT"));
    rootItem->setData(0, ROLE_NODE_TYPE, NODE_ROOT);
    rootItem->setExpanded(true);

    QFont rootFont = rootItem->font(0);
    rootFont.setBold(true);
    rootFont.setPointSize(13);
    rootItem->setFont(0, rootFont);
    rootItem->setForeground(0, QBrush(QColor(QStringLiteral("#333"))));
}

void MainWindow::updateTreeStructure(const QString &name, const QString &path)
{
    if (!treeWidget || treeWidget->topLevelItemCount() == 0) {
        return;
    }

    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *child = root->child(i);
        if (child->text(0) == name && child->data(0, Qt::UserRole).toString() == path) {
            treeWidget->setCurrentItem(child);
            root->setExpanded(true);
            return;
        }
    }

    QTreeWidgetItem *projectItem = new QTreeWidgetItem(root);
    projectItem->setText(0, name);
    projectItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/file.png")));
    projectItem->setData(0, Qt::UserRole, path);
    projectItem->setData(0, ROLE_NODE_TYPE, NODE_PROJECT);
    projectItem->setExpanded(true);

    QFont projectFont = projectItem->font(0);
    projectFont.setBold(true);
    projectFont.setPointSize(13);
    projectItem->setFont(0, projectFont);

    QTreeWidgetItem *infoItem = new QTreeWidgetItem(projectItem);
    infoItem->setText(0, QStringLiteral("工程信息"));
    infoItem->setData(0, Qt::UserRole, path);
    infoItem->setData(0, ROLE_NODE_TYPE, NODE_PROJECT_INFO);
    infoItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/information.png")));

    QFont childFont = infoItem->font(0);
    childFont.setPointSize(13);
    childFont.setBold(false);
    infoItem->setFont(0, childFont);

    QTreeWidgetItem *explosiveItem = new QTreeWidgetItem(projectItem);
    explosiveItem->setText(0, QStringLiteral("炸药参数"));
    explosiveItem->setData(0, Qt::UserRole, path);
    explosiveItem->setData(0, ROLE_NODE_TYPE, NODE_EXPLOSIVE);
    explosiveItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/cailiaocanshu.png")));
    explosiveItem->setFont(0, childFont);

    QTreeWidgetItem *structureItem = new QTreeWidgetItem(projectItem);
    structureItem->setText(0, QStringLiteral("结构参数"));
    structureItem->setData(0, Qt::UserRole, path);
    structureItem->setData(0, ROLE_NODE_TYPE, NODE_STRUCTURE);
    structureItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png")));
    structureItem->setFont(0, childFont);

    QTreeWidgetItem *moldItem = new QTreeWidgetItem(projectItem);
    moldItem->setText(0, QStringLiteral("模具参数"));
    moldItem->setData(0, Qt::UserRole, path);
    moldItem->setData(0, ROLE_NODE_TYPE, NODE_MOLD);
    moldItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png")));
    moldItem->setFont(0, childFont);

    QTreeWidgetItem *boundaryItem = new QTreeWidgetItem(projectItem);
    boundaryItem->setText(0, QStringLiteral("边界条件"));
    boundaryItem->setData(0, Qt::UserRole, path);
    boundaryItem->setData(0, ROLE_NODE_TYPE, NODE_BOUNDARY);
    boundaryItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/fangzhenshezhi.png")));
    boundaryItem->setFont(0, childFont);

    QTreeWidgetItem *simulationItem = new QTreeWidgetItem(projectItem);
    simulationItem->setText(0, QStringLiteral("仿真设置"));
    simulationItem->setData(0, Qt::UserRole, path);
    simulationItem->setData(0, ROLE_NODE_TYPE, NODE_SIMULATION);
    simulationItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/fangzhenshezhi.png")));
    simulationItem->setFont(0, childFont);

    QTreeWidgetItem *checkItem = new QTreeWidgetItem(projectItem);
    checkItem->setText(0, QStringLiteral("参数检查"));
    checkItem->setData(0, Qt::UserRole, path);
    checkItem->setData(0, ROLE_NODE_TYPE, NODE_PARAMETER_CHECK);
    checkItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/check.png")));
    checkItem->setFont(0, childFont);

    QTreeWidgetItem *simulationStartItem = new QTreeWidgetItem(projectItem);
    simulationStartItem->setText(0, QStringLiteral("开始仿真"));
    simulationStartItem->setData(0, Qt::UserRole, path);
    simulationStartItem->setData(0, ROLE_NODE_TYPE, NODE_START_SIMULATION);
    simulationStartItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/start.png")));
    simulationStartItem->setFont(0, childFont);

    treeWidget->setCurrentItem(infoItem);
    root->setExpanded(true);
}

void MainWindow::selectTreeItem(const QString &itemName)
{
    if (!treeWidget) {
        return;
    }

    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    if (!root) {
        return;
    }

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *projectItem = root->child(i);
        if (projectItem->data(0, Qt::UserRole).toString() != currentProject.projectPath) {
            continue;
        }

        for (int j = 0; j < projectItem->childCount(); ++j) {
            QTreeWidgetItem *child = projectItem->child(j);
            if (child->text(0) == itemName) {
                treeWidget->setCurrentItem(child);
                treeWidget->scrollToItem(child);
                return;
            }
        }
    }
}

void MainWindow::loadProjectToUi()
{
    infoWidget->setProjectData(currentProject);
    setWindowTitle(QStringLiteral("浇注XX固化仿真分析工程 - %1").arg(currentProject.projectName));
    selectTreeItem(QStringLiteral("工程信息"));
    stackedWidget->setCurrentWidget(infoWidget);
    updateUIStates();
}

void MainWindow::newProject()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    ProjectConfig config;
    QString error;
    if (!ProjectManager::createProject(dialog.getProjectPath(), dialog.getProjectName(), config, error)) {
        showCenteredMessageBox(this, QMessageBox::Warning, QStringLiteral("创建失败"), error);
        return;
    }

    currentProject = config;
    isProjectLoaded = true;

    updateTreeStructure(config.projectName, config.projectPath);
    startWatchingProject();
    loadProjectToUi();

    showCenteredMessageBox(this, QMessageBox::Information, QStringLiteral("成功"), QStringLiteral("工程创建成功"));
}

void MainWindow::openProject()
{
    OpenProjectDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    bool loadedAny = false;
    for (const QString &path : dialog.getSelectedPaths()) {
        ProjectConfig config;
        if (!ProjectManager::loadProject(path, config)) {
            continue;
        }

        updateTreeStructure(config.projectName, config.projectPath);
        currentProject = config;
        isProjectLoaded = true;
        loadedAny = true;
    }

    if (!loadedAny) {
        return;
    }

    startWatchingProject();
    loadProjectToUi();
}

void MainWindow::saveProject(bool silent)
{
    if (!isProjectLoaded) {
        return;
    }

    infoWidget->getProjectData(currentProject);
    const bool success = ProjectManager::saveProject(currentProject.projectPath, currentProject);

    if (!silent) {
        if (success) {
            showCenteredMessageBox(this, QMessageBox::Information, QStringLiteral("成功"), QStringLiteral("工程已保存。"));
        } else {
            showCenteredMessageBox(this, QMessageBox::Critical, QStringLiteral("错误"), QStringLiteral("工程保存失败，请检查目录权限。"));
        }
    }
}

void MainWindow::exitProject()
{
    if (!isProjectLoaded) {
        return;
    }

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("提示"),
            QStringLiteral("当前仿真正在进行，请等待仿真结束后再关闭工程。")
        );
        return;
    }

    const QString message = QStringLiteral("确认退出工程 \"%1\"？").arg(currentProject.projectName);
    if (showCenteredMessageBox(this, QMessageBox::Question, QStringLiteral("退出工程"), message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    if (root) {
        for (int i = 0; i < root->childCount(); ++i) {
            QTreeWidgetItem *projectItem = root->child(i);
            if (projectItem->data(0, Qt::UserRole).toString() == currentProject.projectPath) {
                delete projectItem;
                break;
            }
        }
    }

    bool hasAnyProject = false;
    if (root) {
        hasAnyProject = root->childCount() > 0;
    }

    if (!hasAnyProject) {
        stopWatchingProject();
        isProjectLoaded = false;
        currentProject = ProjectConfig();
        stackedWidget->setCurrentIndex(0);
        setWindowTitle(QStringLiteral("浇注XX固化仿真分析工程"));
        treeWidget->clearSelection();
    } else if (root && root->childCount() > 0) {
        QTreeWidgetItem *nextProject = root->child(0);

        treeWidget->setCurrentItem(nextProject);
        onTreeItemClicked(nextProject, 0);

        // 工程树发生变化后重新构建目录监控
        startWatchingProject();
    }

    updateUIStates();
}

void MainWindow::projectInfo()
{
    if (!isProjectLoaded) {
        showCenteredMessageBox(this, QMessageBox::Warning, QStringLiteral("提示"), QStringLiteral("请先新建或打开一个工程！"));
        return;
    }

    selectTreeItem(QStringLiteral("工程信息"));
    infoWidget->setProjectData(currentProject);
    stackedWidget->setCurrentWidget(infoWidget);
}

void MainWindow::structureParams()
{
    if (!isProjectLoaded) {
        return;
    }

    StructureConfig config;

    const QString filePath =
        QDir(currentProject.projectPath)
            .filePath(
                QStringLiteral(
                    "config/structure.json"
                )
            );

    if (QFileInfo::exists(filePath)) {

        if (!StructureConfigManager::load(
                currentProject.projectPath,
                config)) {

            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("读取失败"),
                QStringLiteral(
                    "结构参数文件存在，"
                    "但文件内容无效或无法读取。"
                )
            );

            return;
        }

    } else {

        // 新工程第一次进入时使用默认值
        config = StructureConfig();
    }

    structureWidget->setConfig(config);

    selectTreeItem(
        QStringLiteral("结构参数")
    );

    stackedWidget->setCurrentWidget(
        structureWidget
    );
}

void MainWindow::saveStructureParams()
{
    if (!isProjectLoaded) {
        return;
    }

    StructureConfig config = structureWidget->getConfig();
    QString error;

    if (!StructureConfigManager::validate(config, error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("参数错误"),
            error
        );
        return;
    }

    if (!StructureConfigManager::save(currentProject.projectPath, config)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("保存失败"),
            QStringLiteral("结构参数保存失败。")
        );
        return;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("成功"),
        QStringLiteral("结构参数已保存。")
    );
}

void MainWindow::explosiveParams()
{
    if (!isProjectLoaded) {
        return;
    }

    ExplosiveConfig config;

    const QString filePath =
        QDir(currentProject.projectPath)
            .filePath(QStringLiteral("config/explosive.json"));

    if (QFileInfo::exists(filePath)) {
        if (!ExplosiveConfigManager::load(currentProject.projectPath, config)) {
            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("读取失败"),
                QStringLiteral(
                    "炸药参数文件存在，"
                    "但文件内容无效或无法读取。"
                )
            );
            return;
        }
    } else {
        config = ExplosiveConfig();
    }

    explosiveWidget->setConfig(config);
    selectTreeItem(QStringLiteral("炸药参数"));
    stackedWidget->setCurrentWidget(explosiveWidget);
}

void MainWindow::saveExplosiveParams()
{
    if (!isProjectLoaded) {
        return;
    }

    ExplosiveConfig config = explosiveWidget->getConfig();
    QString error;

    if (!ExplosiveConfigManager::validate(config, error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("参数错误"),
            error
        );
        return;
    }

    if (!ExplosiveConfigManager::save(currentProject.projectPath, config)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("保存失败"),
            QStringLiteral("炸药参数保存失败。")
        );
        return;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("成功"),
        QStringLiteral("炸药参数已保存。")
    );
}

void MainWindow::moldParams()
{
    if (!isProjectLoaded) {
        return;
    }

    MoldConfig config;

    const QString filePath =
        QDir(currentProject.projectPath)
            .filePath(QStringLiteral("config/mold.json"));

    if (QFileInfo::exists(filePath)) {
        if (!MoldConfigManager::load(currentProject.projectPath, config)) {
            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("读取失败"),
                QStringLiteral(
                    "模具参数文件存在，"
                    "但文件内容无效或无法读取。"
                )
            );
            return;
        }
    } else {
        config = MoldConfig();
    }

    moldWidget->setConfig(config);
    selectTreeItem(QStringLiteral("模具参数"));
    stackedWidget->setCurrentWidget(moldWidget);
}

void MainWindow::saveMoldParams()
{
    if (!isProjectLoaded) {
        return;
    }

    MoldConfig config = moldWidget->getConfig();
    QString error;

    if (!MoldConfigManager::validate(config, error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("参数错误"),
            error
        );
        return;
    }

    if (!MoldConfigManager::save(currentProject.projectPath, config)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("保存失败"),
            QStringLiteral("模具参数保存失败。")
        );
        return;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("成功"),
        QStringLiteral("模具参数已保存。")
    );
}

void MainWindow::boundaryParams()
{
    if (!isProjectLoaded) {
        return;
    }

    BoundaryConfig config;

    const QString filePath =
        QDir(currentProject.projectPath)
            .filePath(QStringLiteral("config/boundary.json"));

    if (QFileInfo::exists(filePath)) {
        if (!BoundaryConfigManager::load(currentProject.projectPath, config)) {
            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("读取失败"),
                QStringLiteral(
                    "边界条件文件存在，"
                    "但文件内容无效或无法读取。"
                )
            );
            return;
        }
    } else {
        config = BoundaryConfig();
    }

    boundaryWidget->setConfig(config);
    selectTreeItem(QStringLiteral("边界条件"));
    stackedWidget->setCurrentWidget(boundaryWidget);
}

void MainWindow::saveBoundaryParams()
{
    if (!isProjectLoaded) {
        return;
    }

    BoundaryConfig config = boundaryWidget->getConfig();
    QString error;

    if (!BoundaryConfigManager::validate(config, error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("参数错误"),
            error
        );
        return;
    }

    if (!BoundaryConfigManager::save(currentProject.projectPath, config)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("保存失败"),
            QStringLiteral("边界条件保存失败。")
        );
        return;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("成功"),
        QStringLiteral("边界条件已保存。")
    );
}

void MainWindow::simulationParams()
{
    if (!isProjectLoaded) {
        return;
    }

    SimulationConfig config;

    const QString filePath =
        QDir(currentProject.projectPath)
            .filePath(QStringLiteral("config/simulation.json"));

    if (QFileInfo::exists(filePath)) {
        if (!SimulationConfigManager::load(currentProject.projectPath, config)) {
            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("读取失败"),
                QStringLiteral(
                    "仿真设置文件存在，"
                    "但文件内容无效或无法读取。"
                )
            );
            return;
        }
    } else {
        config = SimulationConfig();
    }

    simulationWidget->setConfig(config);
    selectTreeItem(QStringLiteral("仿真设置"));
    stackedWidget->setCurrentWidget(simulationWidget);
}

void MainWindow::saveSimulationParams()
{
    if (!isProjectLoaded) {
        return;
    }

    SimulationConfig config = simulationWidget->getConfig();
    QString error;

    if (!SimulationConfigManager::validate(config, error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("参数错误"),
            error
        );
        return;
    }

    if (!SimulationConfigManager::save(currentProject.projectPath, config)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("保存失败"),
            QStringLiteral("仿真设置保存失败。")
        );
        return;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("成功"),
        QStringLiteral("仿真设置已保存。")
    );
}

void MainWindow::checkParams()
{
    if (!isProjectLoaded) {
        return;
    }

    parameterCheckWidget->refresh(currentProject);
    selectTreeItem(QStringLiteral("参数检查"));
    stackedWidget->setCurrentWidget(parameterCheckWidget);
}

void MainWindow::generateFiles()
{
    if (!isProjectLoaded) {
        return;
    }

    StructureConfig structure;
    if (!StructureConfigManager::load(currentProject.projectPath, structure)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("无法生成"),
            QStringLiteral("请先填写并保存结构参数。")
        );
        return;
    }

    ExplosiveConfig explosive;
    if (!ExplosiveConfigManager::load(currentProject.projectPath, explosive)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("无法生成"),
            QStringLiteral("请先填写并保存炸药参数。")
        );
        return;
    }

    MoldConfig mold;
    if (!MoldConfigManager::load(currentProject.projectPath, mold)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("无法生成"),
            QStringLiteral("请先填写并保存模具参数。")
        );
        return;
    }

    BoundaryConfig boundary;
    if (!BoundaryConfigManager::load(currentProject.projectPath, boundary)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("无法生成"),
            QStringLiteral("请先填写并保存边界条件。")
        );
        return;
    }

    SimulationConfig simulation;
    if (!SimulationConfigManager::load(currentProject.projectPath, simulation)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("无法生成"),
            QStringLiteral("请先填写并保存仿真设置。")
        );
        return;
    }

    QString error;
    if (!AbaqusFileGenerator::generate(
            currentProject.projectPath,
            structure,
            explosive,
            mold,
            boundary,
            simulation,
            error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("生成失败"),
            error
        );
        return;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("成功"),
        QStringLiteral("Abaqus 文件生成成功。")
    );
}

bool MainWindow::checkSimulationReady(QString &errorMessage)
{
    const QString projectDir = currentProject.projectPath;
    const QString abaqusDir =
        QDir(projectDir).filePath(QStringLiteral("abaqus"));

    const QStringList files = {
        QDir(abaqusDir).filePath(QStringLiteral("t0.py")),
        QDir(abaqusDir).filePath(QStringLiteral("t1.py")),
        QDir(abaqusDir).filePath(QStringLiteral("335K.for"))
    };

    for (const QString &file : files) {
        if (!QFile::exists(file)) {
            errorMessage =
                QStringLiteral("缺少文件:\n%1").arg(file);
            return false;
        }
    }

    const QString abaqusPath = SettingsDialog::getAbaqusPath();
    if (abaqusPath.isEmpty() || !QFile::exists(abaqusPath)) {
        errorMessage = QStringLiteral("Abaqus路径无效");
        return false;
    }

    return true;
}

void MainWindow::startSimulation()
{
    if (!isProjectLoaded) {
        return;
    }

    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("提示"),
            QStringLiteral("仿真正在进行中，请稍候。")
        );
        return;
    }

    QString error;
    if (!checkSimulationReady(error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("仿真无法启动"),
            error
        );
        return;
    }

    // currentProject.projectPath 已是工程根目录，例如 D:/Test01
    const QString projectDir = currentProject.projectPath;
    const QString abaqusDir =
        QDir(projectDir).filePath(QStringLiteral("abaqus"));
    const QString t0Path =
        QDir(abaqusDir).filePath(QStringLiteral("t0.py"));
    const QString t1Path =
        QDir(abaqusDir).filePath(QStringLiteral("t1.py"));

    const QDateTime generatedTime = QFileInfo(t0Path).lastModified();
    const QStringList configFiles = {
        QDir(projectDir).filePath(QStringLiteral("config/structure.json")),
        QDir(projectDir).filePath(QStringLiteral("config/explosive.json")),
        QDir(projectDir).filePath(QStringLiteral("config/mold.json")),
        QDir(projectDir).filePath(QStringLiteral("config/boundary.json")),
        QDir(projectDir).filePath(QStringLiteral("config/simulation.json"))
    };

    for (const QString &configFile : configFiles) {
        const QFileInfo info(configFile);
        if (info.exists() && info.lastModified() > generatedTime) {
            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("需要重新生成"),
                QStringLiteral(
                    "参数在 Abaqus 文件生成后发生过修改，"
                    "请重新生成文件后再开始仿真。"
                )
            );
            return;
        }
    }

    const QString abaqusPath = SettingsDialog::getAbaqusPath();

    const QString logsDir =
        QDir(projectDir).filePath(QStringLiteral("logs"));
    QDir().mkpath(logsDir);
    const QString t0LogPath =
        QDir(logsDir).filePath(QStringLiteral("t0.log"));
    const QString t1LogPath =
        QDir(logsDir).filePath(QStringLiteral("t1.log"));

    auto clearLogFile = [](const QString &logPath) {
        QFile logFile(logPath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            logFile.close();
        }
    };
    auto appendProcessLog =
        [this](const QString &logPath, const QByteArray &data, bool isError) {
            if (data.isEmpty()) {
                return;
            }
            QFile logFile(logPath);
            if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
                logFile.write(data);
            }
            const QString text = QString::fromLocal8Bit(data);
            if (text.contains(QStringLiteral("License Manager"))
                || text.contains(QStringLiteral("checked out"))) {
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[ABAQUS] ") + text
                );
            } else if (isError) {
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[ERROR] ") + text
                );
            } else {
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[SYS] ") + text
                );
            }
        };

    clearLogFile(t0LogPath);
    clearLogFile(t1LogPath);

    QFile::remove(
        QDir(abaqusDir).filePath(QStringLiteral("t0_finished.flag"))
    );
    QFile::remove(
        QDir(abaqusDir).filePath(QStringLiteral("t1_finished.flag"))
    );

    simulationMonitorWidget->setProgress(0);
    simulationMonitorWidget->setJob(QString());
    simulationMonitorWidget->setStatus(
        QStringLiteral("正在启动 Abaqus...")
    );
    simulationMonitorWidget->setPhase(
        QStringLiteral("阶段: 模型建立(t0)")
    );
    simulationMonitorWidget->appendLog(
        QStringLiteral("[SYS] 开始仿真")
    );
    stackedWidget->setCurrentWidget(simulationMonitorWidget);

    if (abaqusProcess) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
    }

    // ---------- 阶段 1：t0.py ----------
    abaqusProcess = new QProcess(this);
    abaqusProcess->setWorkingDirectory(abaqusDir);

    connect(
        abaqusProcess,
        &QProcess::readyReadStandardOutput,
        this,
        [this, t0LogPath, appendProcessLog]() {
            if (abaqusProcess) {
                appendProcessLog(
                    t0LogPath,
                    abaqusProcess->readAllStandardOutput(),
                    false
                );
            }
        }
    );

    connect(
        abaqusProcess,
        &QProcess::readyReadStandardError,
        this,
        [this, t0LogPath, appendProcessLog]() {
            if (abaqusProcess) {
                appendProcessLog(
                    t0LogPath,
                    abaqusProcess->readAllStandardError(),
                    true
                );
            }
        }
    );

    connect(
        abaqusProcess,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        [this, t1Path, abaqusPath, abaqusDir, projectDir, t1LogPath, appendProcessLog](
            int exitCode,
            QProcess::ExitStatus
        ) {
            if (abaqusProcess) {
                abaqusProcess->deleteLater();
                abaqusProcess = nullptr;
            }

            if (exitCode != 0) {
                simulationMonitorWidget->setStatus(
                    QStringLiteral("t0.py 执行失败")
                );
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[SYS] t0.py 执行失败")
                );
                showCenteredMessageBox(
                    this,
                    QMessageBox::Critical,
                    QStringLiteral("错误"),
                    QStringLiteral("t0.py 执行失败。")
                );
                return;
            }

            const QString caePath =
                QDir(abaqusDir).filePath(QStringLiteral("guhua.cae"));
            if (!QFile::exists(caePath)) {
                simulationMonitorWidget->setStatus(
                    QStringLiteral("未生成 guhua.cae")
                );
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[SYS] t0 执行完成，但未生成 guhua.cae")
                );
                showCenteredMessageBox(
                    this,
                    QMessageBox::Critical,
                    QStringLiteral("错误"),
                    QStringLiteral("t0 执行完成，但未生成 guhua.cae。")
                );
                return;
            }

            const QString flagPath =
                QDir(abaqusDir).filePath(QStringLiteral("t0_finished.flag"));
            if (!QFile::exists(flagPath)) {
                simulationMonitorWidget->setStatus(
                    QStringLiteral("t0执行失败")
                );
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[SYS] t0未生成完成标志")
                );
                showCenteredMessageBox(
                    this,
                    QMessageBox::Critical,
                    QStringLiteral("错误"),
                    QStringLiteral("t0 未正常完成（缺少完成标志）。")
                );
                return;
            }

            simulationMonitorWidget->appendLog(
                QStringLiteral("[SYS] 模型生成完成，启动 t1.py")
            );
            simulationMonitorWidget->setStatus(
                QStringLiteral("Abaqus 求解中")
            );
            simulationMonitorWidget->setPhase(
                QStringLiteral("阶段: Abaqus求解(t1)")
            );

            // ---------- 阶段 2：t1.py（t0 成功后新建进程）----------
            abaqusProcess = new QProcess(this);
            abaqusProcess->setWorkingDirectory(abaqusDir);

            connect(
                abaqusProcess,
                &QProcess::readyReadStandardOutput,
                this,
                [this, t1LogPath, appendProcessLog]() {
                    if (abaqusProcess) {
                        appendProcessLog(
                            t1LogPath,
                            abaqusProcess->readAllStandardOutput(),
                            false
                        );
                    }
                }
            );

            connect(
                abaqusProcess,
                &QProcess::readyReadStandardError,
                this,
                [this, t1LogPath, appendProcessLog]() {
                    if (abaqusProcess) {
                        appendProcessLog(
                            t1LogPath,
                            abaqusProcess->readAllStandardError(),
                            true
                        );
                    }
                }
            );

            const QString jobName =
                QDir(projectDir).dirName() + QStringLiteral("_Job");
            simulationMonitorWidget->setJob(jobName);
            simulationMsgPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".msg"));
            simulationStaPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".sta"));
            simulationDatPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".dat"));
            simulationTotalTime = loadSimulationTotalTime();
            lastMsgCache.clear();
            lastStaCache.clear();

            QFile::remove(simulationMsgPath);
            QFile::remove(simulationStaPath);
            QFile::remove(simulationDatPath);

            const QString odbPath =
                QDir(abaqusDir).filePath(jobName + QStringLiteral(".odb"));
            const QString t1FlagPath =
                QDir(abaqusDir).filePath(QStringLiteral("t1_finished.flag"));

            connect(
                abaqusProcess,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                [this, odbPath, t1FlagPath](int t1ExitCode, QProcess::ExitStatus) {
                    if (abaqusProcess) {
                        abaqusProcess->deleteLater();
                        abaqusProcess = nullptr;
                    }

                    if (simulationTimer) {
                        simulationTimer->stop();
                    }

                    if (t1ExitCode != 0) {
                        simulationMonitorWidget->setStatus(
                            QStringLiteral("t1.py 执行失败")
                        );
                        simulationMonitorWidget->appendLog(
                            QStringLiteral("[SYS] t1.py 执行失败")
                        );
                        showCenteredMessageBox(
                            this,
                            QMessageBox::Warning,
                            QStringLiteral("错误"),
                            QStringLiteral("t1.py 执行失败。")
                        );
                        return;
                    }

                    if (!QFile::exists(odbPath)) {
                        simulationMonitorWidget->setStatus(
                            QStringLiteral("Abaqus 未生成计算结果")
                        );
                        simulationMonitorWidget->appendLog(
                            QStringLiteral("[SYS] Abaqus 未生成计算结果 ODB")
                        );
                        showCenteredMessageBox(
                            this,
                            QMessageBox::Warning,
                            QStringLiteral("错误"),
                            QStringLiteral(
                                "Abaqus 未生成计算结果。\n\n"
                                "可能原因：\n"
                                "1. 用户中断计算\n"
                                "2. Abaqus 求解失败\n"
                                "3. 用户子程序错误"
                            )
                        );
                        return;
                    }

                    if (!QFile::exists(t1FlagPath)) {
                        simulationMonitorWidget->setStatus(
                            QStringLiteral("t1未正常完成")
                        );
                        simulationMonitorWidget->appendLog(
                            QStringLiteral("[SYS] t1未生成完成标志")
                        );
                        showCenteredMessageBox(
                            this,
                            QMessageBox::Warning,
                            QStringLiteral("错误"),
                            QStringLiteral("t1 未正常完成（缺少完成标志）。")
                        );
                        return;
                    }

                    simulationMonitorWidget->setStatus(
                        QStringLiteral("固化仿真完成")
                    );
                    simulationMonitorWidget->setPhase(
                        QStringLiteral("阶段: 完成")
                    );
                    simulationMonitorWidget->setProgress(100);
                    simulationMonitorWidget->appendLog(
                        QStringLiteral("[SYS] 固化仿真完成")
                    );
                    showCenteredMessageBox(
                        this,
                        QMessageBox::Information,
                        QStringLiteral("完成"),
                        QStringLiteral("固化仿真完成。")
                    );
                }
            );

            simulationMonitorWidget->appendLog(
                QStringLiteral("[SYS] 启动 t1.py")
            );
            abaqusProcess->start(
                abaqusPath,
                QStringList()
                    << QStringLiteral("cae")
                    << QStringLiteral("script=") + t1Path
            );

            if (!abaqusProcess->waitForStarted(10000)) {
                abaqusProcess->deleteLater();
                abaqusProcess = nullptr;
                if (simulationTimer) {
                    simulationTimer->stop();
                }
                simulationMonitorWidget->setStatus(
                    QStringLiteral("无法启动 t1.py")
                );
                showCenteredMessageBox(
                    this,
                    QMessageBox::Warning,
                    QStringLiteral("错误"),
                    QStringLiteral("无法启动 t1.py。")
                );
                return;
            }

            if (!simulationTimer) {
                simulationTimer = new QTimer(this);
                connect(
                    simulationTimer,
                    &QTimer::timeout,
                    this,
                    &MainWindow::updateAbaqusLog
                );
            }
            simulationTimer->start(1000);
        }
    );

    simulationMonitorWidget->appendLog(
        QStringLiteral("[SYS] 启动 t0.py")
    );
    abaqusProcess->start(
        abaqusPath,
        QStringList()
            << QStringLiteral("cae")
            << QStringLiteral("script=") + t0Path
    );

    if (!abaqusProcess->waitForStarted(10000)) {
        abaqusProcess->deleteLater();
        abaqusProcess = nullptr;
        simulationMonitorWidget->setStatus(
            QStringLiteral("无法启动 t0.py")
        );
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("错误"),
            QStringLiteral("无法启动 t0.py。")
        );
        return;
    }

    simulationMonitorWidget->setStatus(
        QStringLiteral("正在执行 Abaqus")
    );
}

void MainWindow::updateAbaqusLog()
{
    // 读取顺序：msg（求解过程）→ sta（状态摘要）
    readAbaqusLogFile(
        simulationMsgPath,
        QStringLiteral("[MSG]"),
        lastMsgCache
    );
    readAbaqusLogFile(
        simulationStaPath,
        QStringLiteral("[STA]"),
        lastStaCache
    );
}

void MainWindow::readAbaqusLogFile(
    const QString &path,
    const QString &tag,
    QString &lastContent
)
{
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    const QStringList lines =
        QString::fromLocal8Bit(file.readAll())
            .split(QStringLiteral("\n"), Qt::SkipEmptyParts);
    file.close();

    if (lines.isEmpty()) {
        return;
    }

    const int start = qMax(0, lines.size() - 10);
    QStringList recentLines;
    for (int i = start; i < lines.size(); ++i) {
        const QString line = lines[i].trimmed();
        if (!line.isEmpty()) {
            recentLines.append(line);
        }
    }

    if (recentLines.isEmpty()) {
        return;
    }

    const QString content = recentLines.join(QLatin1Char('\n'));
    if (content == lastContent) {
        return;
    }
    lastContent = content;

    for (const QString &line : recentLines) {
        simulationMonitorWidget->appendLog(tag + QStringLiteral(" ") + line);
    }
}

double MainWindow::loadSimulationTotalTime()
{
    SimulationConfig config;
    if (SimulationConfigManager::load(currentProject.projectPath, config)) {
        return config.timeLength;
    }
    return 0.0;
}

void MainWindow::settings()
{
    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::help()
{
    showCenteredMessageBox(this, QMessageBox::Information, QStringLiteral("帮助"), QStringLiteral("帮助文档功能正在开发中..."));
}

QMessageBox::StandardButton MainWindow::showCenteredMessageBox(
    QWidget *parent,
    QMessageBox::Icon icon,
    const QString &title,
    const QString &text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    QMessageBox msgBox(icon, title, text, buttons, parent);
    if (defaultButton != QMessageBox::NoButton) {
        msgBox.setDefaultButton(defaultButton);
    }
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.exec();
    return msgBox.standardButton(msgBox.clickedButton());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (abaqusProcess
        && abaqusProcess->state() != QProcess::NotRunning) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("提示"),
            QStringLiteral("当前仿真正在进行，请等待仿真结束后再退出软件。")
        );
        event->ignore();
        return;
    }

    if (showCenteredMessageBox(this, QMessageBox::Question, QStringLiteral("退出"), QStringLiteral("确定要退出吗？"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::onTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    onTreeItemClicked(item, column);
}

void MainWindow::onTreeItemClicked(
    QTreeWidgetItem *item,
    int column)
{
    Q_UNUSED(column);

    if (!item) {
        return;
    }

    const QString nodeType =
        item->data(
            0,
            ROLE_NODE_TYPE
        ).toString();

    // 根节点不代表具体工程
    if (nodeType == NODE_ROOT) {
        return;
    }

    QTreeWidgetItem *projectItem = nullptr;

    if (nodeType == NODE_PROJECT) {
        projectItem = item;
    }
    else if (nodeType == NODE_PROJECT_INFO
             || nodeType == NODE_STRUCTURE
             || nodeType == NODE_EXPLOSIVE
             || nodeType == NODE_MOLD
             || nodeType == NODE_BOUNDARY
             || nodeType == NODE_SIMULATION
             || nodeType == NODE_PARAMETER_CHECK
             || nodeType == NODE_START_SIMULATION) {
        projectItem = item->parent();
    }
    else {
        return;
    }

    if (!projectItem) {
        return;
    }

    const QString path =
        projectItem->data(
            0,
            Qt::UserRole
        ).toString();

    if (path.isEmpty()) {
        return;
    }

    // 如果切换到了另一个工程，重新加载该工程
    if (!isProjectLoaded ||
        path != currentProject.projectPath) {

        ProjectConfig config;

        if (!ProjectManager::loadProject(
                path,
                config)) {

            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("错误"),
                QStringLiteral(
                    "该工程文件夹已不存在"
                    "或不是有效的工程。"
                )
            );

            delete projectItem;
            startWatchingProject();
            return;
        }

        currentProject = config;
        isProjectLoaded = true;

        updateUIStates();
    }

    // 更新窗口标题
    setWindowTitle(
        QStringLiteral(
            "浇注XX固化仿真分析工程 - %1"
        ).arg(currentProject.projectName)
    );

    if (nodeType == NODE_STRUCTURE) {
        structureParams();
        return;
    }

    if (nodeType == NODE_EXPLOSIVE) {
        explosiveParams();
        return;
    }

    if (nodeType == NODE_MOLD) {
        moldParams();
        return;
    }

    if (nodeType == NODE_BOUNDARY) {
        boundaryParams();
        return;
    }

    if (nodeType == NODE_SIMULATION) {
        simulationParams();
        return;
    }

    if (nodeType == NODE_PARAMETER_CHECK) {
        checkParams();
        return;
    }

    if (nodeType == NODE_START_SIMULATION) {
        startSimulation();
        return;
    }

    infoWidget->setProjectData(currentProject);
    stackedWidget->setCurrentWidget(infoWidget);

    if (nodeType == NODE_PROJECT) {
        selectTreeItem(
            QStringLiteral("工程信息")
        );
    }
}

void MainWindow::startWatchingProject()
{
    if (!fileWatcher) {
        return;
    }

    if (!fileWatcher->directories().isEmpty()) {
        fileWatcher->removePaths(fileWatcher->directories());
    }

    QSet<QString> uniquePaths;
    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    if (!root) {
        return;
    }

    for (int i = 0; i < root->childCount(); ++i) {
        const QString path = root->child(i)->data(0, Qt::UserRole).toString();
        if (path.isEmpty()) {
            continue;
        }

        const QFileInfo info(path);
        const QString parentPath = info.absolutePath();
        if (QFileInfo(parentPath).exists()) {
            uniquePaths.insert(parentPath);
        }
        if (info.exists()) {
            uniquePaths.insert(path);
        }
    }

    if (!uniquePaths.isEmpty()) {
        fileWatcher->addPaths(uniquePaths.values());
    }
}

void MainWindow::stopWatchingProject()
{
    if (fileWatcher && !fileWatcher->directories().isEmpty()) {
        fileWatcher->removePaths(fileWatcher->directories());
    }
}

void MainWindow::onProjectDirectoryChanged()
{
    QList<QTreeWidgetItem *> itemsToDelete;
    bool currentProjectDeleted = false;

    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    if (!root) {
        return;
    }

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *projectItem = root->child(i);
        const QString path = projectItem->data(0, Qt::UserRole).toString();
        if (!path.isEmpty() && !QFileInfo(path).exists()) {
            itemsToDelete.append(projectItem);
            if (isProjectLoaded && path == currentProject.projectPath) {
                currentProjectDeleted = true;
            }
        }
    }

    for (QTreeWidgetItem *item : itemsToDelete) {
        delete item;
    }

    if (currentProjectDeleted) {
        stopWatchingProject();
        isProjectLoaded = false;
        currentProject = ProjectConfig();
        stackedWidget->setCurrentIndex(0);
        setWindowTitle(QStringLiteral("浇注XX固化仿真分析工程"));
        updateUIStates();
        startWatchingProject();
    }
}

void MainWindow::updateUIStates()
{
    for (QAction *act : projectDependentActions) {
        act->setEnabled(isProjectLoaded);
    }
}
