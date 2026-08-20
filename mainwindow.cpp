#include "mainwindow.h"

#include "NewProjectDialog.h"
#include "OpenProjectDialog.h"
#include "SettingsDialog.h"
#include "StructureConfigManager.h"
#include "ExplosiveConfigManager.h"
#include "MoldConfigManager.h"
#include "BoundaryConfigManager.h"
#include "SimulationConfigManager.h"
#include "AbaqusFileGenerator.h"

#include <QBrush>
#include <QCloseEvent>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGridLayout>
#include <QIcon>
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

    addBtn(QStringLiteral("退出工程"), QStringLiteral(":/new/prefix1/toolbar_picture/close.png"), &MainWindow::exitProject);

    toolBar->addSeparator();

    addBtn(QStringLiteral("工程信息"), QStringLiteral(":/new/prefix1/toolbar_picture/information.png"), &MainWindow::projectInfo);
    addBtn(QStringLiteral("炸药参数"), QStringLiteral(":/new/prefix1/toolbar_picture/cailiaocanshu.png"), &MainWindow::explosiveParams);
    addBtn(QStringLiteral("结构参数"), QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png"), &MainWindow::structureParams);
    addBtn(QStringLiteral("模具参数"), QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png"), &MainWindow::moldParams);
    addBtn(QStringLiteral("边界条件"), QStringLiteral(":/new/prefix1/toolbar_picture/fangzhenshezhi.png"), &MainWindow::boundaryParams);
    addBtn(QStringLiteral("仿真设置"), QStringLiteral(":/new/prefix1/toolbar_picture/fangzhenshezhi.png"), &MainWindow::simulationParams);

    toolBar->addSeparator();

    addPlaceholderBtn(QStringLiteral("参数检查"), QStringLiteral(":/new/prefix1/toolbar_picture/check.png"));
    addBtn(QStringLiteral("生成文件"), QStringLiteral(":/new/prefix1/toolbar_picture/file.png"), &MainWindow::generateFiles);
    addPlaceholderBtn(QStringLiteral("开始仿真"), QStringLiteral(":/new/prefix1/toolbar_picture/start.png"));
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
    infoItem->setFont(0, childFont);

    QTreeWidgetItem *structureItem = new QTreeWidgetItem(projectItem);
    structureItem->setText(0, QStringLiteral("结构参数"));
    structureItem->setData(0, Qt::UserRole, path);
    structureItem->setData(0, ROLE_NODE_TYPE, NODE_STRUCTURE);
    structureItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/jiegoucanshu.png")));
    structureItem->setFont(0, childFont);

    QTreeWidgetItem *explosiveItem = new QTreeWidgetItem(projectItem);
    explosiveItem->setText(0, QStringLiteral("炸药参数"));
    explosiveItem->setData(0, Qt::UserRole, path);
    explosiveItem->setData(0, ROLE_NODE_TYPE, NODE_EXPLOSIVE);
    explosiveItem->setIcon(0, QIcon(QStringLiteral(":/new/prefix1/toolbar_picture/cailiaocanshu.png")));
    explosiveItem->setFont(0, childFont);

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
             || nodeType == NODE_SIMULATION) {
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
