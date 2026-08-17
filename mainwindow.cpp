#include "mainwindow.h"
#include "NewProjectDialog.h"
#include "OpenProjectDialog.h"
#include "ProjectManager.h"
#include "KneadingExplosiveWidget.h"
#include "ExtrusionExplosiveWidget.h"
#include "KneadingBladeWidget.h"
#include "ExtrusionScrewWidget.h"
#include "KneadingBoundaryWidget.h"
#include "ExtrusionBoundaryWidget.h"
#include "KneadingSimulationWidget.h"
#include "ExtrusionSimulationWidget.h"

#include <QPainter>
#include <QPixmap>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

#include <QHBoxLayout>
#include <QDateTime>
#include <QToolBar>
#include <QTextStream>
#include <QRegularExpression>
#include <QThread>
#include <QMenu>
#include <QStandardPaths>

namespace {
// 辅助函数：创建信息行
QHBoxLayout* createInfoRow(const QString &label, const QString &value) {
    QHBoxLayout *row = new QHBoxLayout();
    QLabel *lblKey = new QLabel(label);
    lblKey->setFixedWidth(180);
    lblKey->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lblKey->setMinimumHeight(38);
    lblKey->setStyleSheet("font-family: 'Microsoft YaHei'; color: #000; font-weight: bold; font-size: 16px;");

    QLineEdit *leValue = new QLineEdit(value);
    leValue->setReadOnly(true);
    leValue->setMinimumHeight(38);
    leValue->setStyleSheet("border: none; background: transparent; color: #333; margin-left: 5px; font-family: 'Microsoft YaHei'; font-size: 16px; font-weight: bold;");

    row->addWidget(lblKey);
    row->addWidget(leValue);
    row->setContentsMargins(0, 0, 0, 0);
    return row;
}

// 辅助函数：创建分组框
QGroupBox* createSectionBox(const QString &title) {
    QGroupBox *box = new QGroupBox(title);
    box->setStyleSheet("QGroupBox { font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 18px; border: 1px solid #ccc; border-radius: 6px; margin-top: 10px; padding-top: 15px; } QGroupBox::title { left: 10px; padding: 0 5px; }");
    QVBoxLayout *layout = new QVBoxLayout(box);
    layout->setSpacing(2);
    layout->setContentsMargins(10, 15, 10, 10);
    return box;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("仿真工程管理系统");
    setMinimumSize(1300, 800);
    resize(1300, 800);
    setupUi();

    this->setStyleSheet(R"(
    QMainWindow {
        border-image: url(:/new/prefix1/toolbar_picture/back.png) 0 0 0 0 stretch stretch;
    }
    )");

    fileWatcher = new QFileSystemWatcher(this);

    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(100);

    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, [this](){ debounceTimer->start(); });
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, [this](){ debounceTimer->start(); });
    connect(debounceTimer, &QTimer::timeout, this, &MainWindow::onProjectDirectoryChanged);

    // 连接指挥官信号
    connect(&SimulationManager::instance(), &SimulationManager::logReceived,
            this, &MainWindow::onSimulationLogReceived);
    connect(&SimulationManager::instance(), &SimulationManager::progressUpdated,
            this, &MainWindow::onSimulationProgress);
    connect(&SimulationManager::instance(), &SimulationManager::taskFinished,
            this, &MainWindow::onSimulationFinished);

    updateUIStates();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    createPureStyleToolBar();

    centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    centralWidget->setStyleSheet("QWidget#centralWidget { background: transparent; }");

    mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 左侧栏
    QWidget *leftWidget = new QWidget(centralWidget);
    leftWidget->setFixedWidth(300);
    leftWidget->setStyleSheet("background-color: #ffffff; border-right: 1px solid #e6e6e6;");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    createTreeWidget();
    leftLayout->addWidget(treeWidget);

    // 右侧内容
    QWidget *rightWidget = new QWidget(centralWidget);
    rightWidget->setStyleSheet("background: transparent;");

    QGridLayout *rightLayout = new QGridLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    stackedWidget = new QStackedWidget(rightWidget);
    stackedWidget->setAttribute(Qt::WA_TranslucentBackground);

    // Page 0: 标题
    titleLabel = new QLabel("压装炸药制造工艺安全仿真分析软件", stackedWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet("font-family: 'Microsoft YaHei'; font-size: 60px; color: #333; font-weight: bold; background: transparent; padding: 20px;");
    stackedWidget->addWidget(titleLabel);

    // Page 1-N: 子页面
    infoWidget = new ProjectInfoWidget(stackedWidget);
    stackedWidget->addWidget(infoWidget);

    kneadingExplosiveWidget = new KneadingExplosiveWidget(stackedWidget);
    stackedWidget->addWidget(kneadingExplosiveWidget);

    extrusionExplosiveWidget = new ExtrusionExplosiveWidget(stackedWidget);
    stackedWidget->addWidget(extrusionExplosiveWidget);

    kneadingBladeWidget = new KneadingBladeWidget(stackedWidget);
    stackedWidget->addWidget(kneadingBladeWidget);

    extrusionScrewWidget = new ExtrusionScrewWidget(stackedWidget);
    stackedWidget->addWidget(extrusionScrewWidget);

    kneadingBoundaryWidget = new KneadingBoundaryWidget(stackedWidget);
    stackedWidget->addWidget(kneadingBoundaryWidget);

    extrusionBoundaryWidget = new ExtrusionBoundaryWidget(stackedWidget);
    stackedWidget->addWidget(extrusionBoundaryWidget);

    kneadingSimulationWidget = new KneadingSimulationWidget(stackedWidget);
    stackedWidget->addWidget(kneadingSimulationWidget);

    extrusionSimulationWidget = new ExtrusionSimulationWidget(stackedWidget);
    stackedWidget->addWidget(extrusionSimulationWidget);



    setupWidgetCallbacks();

    // 日志监控页
    setupLogOutput();
    stackedWidget->addWidget(simulationMonitorWidget);

    rightLayout->addWidget(stackedWidget, 0, 0, 1, 1);

    mainLayout->addWidget(leftWidget);
    mainLayout->addWidget(rightWidget);
    setCentralWidget(centralWidget);
}

void MainWindow::setupLogOutput() {
    simulationMonitorWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(simulationMonitorWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 顶部标题栏
    QWidget *header = new QWidget();
    header->setFixedHeight(60);
    header->setStyleSheet("background-color: #f8f9fa; border-bottom: 1px solid #ddd;");

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *title = new QLabel("仿真运行监控");
    title->setStyleSheet("font-family: 'Microsoft YaHei'; font-size: 20px; font-weight: bold; color: #333;");
    headerLayout->addWidget(title);

    headerLayout->addStretch();

    // 终止仿真按钮
    QPushButton *btnStop = new QPushButton("终止仿真");
    btnStop->setFixedSize(100, 36);
    btnStop->setCursor(Qt::PointingHandCursor);
    btnStop->setStyleSheet(
        "QPushButton { background-color: #ff4d4f; color: white; border: none; border-radius: 4px; font-weight: bold; font-size: 14px; } "
        "QPushButton:hover { background-color: #ff7875; } "
        "QPushButton:pressed { background-color: #d9363e; }"
        );
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::terminateSimulation);
    headerLayout->addWidget(btnStop);

    layout->addWidget(header);

    // 进度条区域
    QWidget *progressContainer = new QWidget();
    progressContainer->setFixedHeight(50);
    progressContainer->setStyleSheet("background-color: white; border-bottom: 1px solid #eee;");
    QHBoxLayout *progressLayout = new QHBoxLayout(progressContainer);
    progressLayout->setContentsMargins(20, 0, 20, 0);

    statusLabel = new QLabel("准备就绪");
    statusLabel->setStyleSheet("font-family: 'Microsoft YaHei'; font-size: 14px; color: #666;");
    progressLayout->addWidget(statusLabel);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    progressBar->setFixedHeight(20);
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #ddd; border-radius: 10px; text-align: center; background: #eee; }"
        "QProgressBar::chunk { background-color: #1890ff; border-radius: 9px; }"
        );
    progressLayout->addWidget(progressBar);

    layout->addWidget(progressContainer);

    // 日志文本框
    logOutput = new QTextEdit();
    logOutput->setReadOnly(true);
    logOutput->setFont(QFont("Courier New", 10));
    logOutput->setStyleSheet("QTextEdit { background-color: #f5f5f5; border: none; padding: 10px; color: #333; }");
    logOutput->setPlaceholderText("等待仿真启动...");

    // 启用自定义右键菜单
    logOutput->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(logOutput, &QTextEdit::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu *menu = new QMenu(this);
        menu->setStyleSheet(
            "QMenu { background-color: #ffffff; border: 1px solid #cccccc; padding: 5px; }"
            "QMenu::item { padding: 5px 20px; color: #333333; background-color: transparent; }"
            "QMenu::item:selected { background-color: #e6f7ff; color: #1890ff; }"
            );

        QAction *actCopy = new QAction("复制", menu);
        actCopy->setEnabled(logOutput->textCursor().hasSelection());
        connect(actCopy, &QAction::triggered, logOutput, &QTextEdit::copy);
        menu->addAction(actCopy);

        QAction *actSelectAll = new QAction("全选", menu);
        connect(actSelectAll, &QAction::triggered, logOutput, &QTextEdit::selectAll);
        menu->addAction(actSelectAll);

        menu->addSeparator();
        QAction *actClear = new QAction("清空日志", menu);
        connect(actClear, &QAction::triggered, logOutput, &QTextEdit::clear);
        menu->addAction(actClear);

        menu->exec(logOutput->mapToGlobal(pos));
        delete menu;
    });

    layout->addWidget(logOutput);
}

void MainWindow::setupWidgetCallbacks() {
    auto checkProject = [this]() -> bool {
        if (!isProjectLoaded || currentProject.projectPath.isEmpty()) {
            showCenteredMessageBox(this, QMessageBox::Warning, "警告", "请先新建或打开一个工程！");
            return false;
        }
        return true;
    };

    auto bindWidget = [&](auto* widget, auto saveFunc, const QString& successMsg) {
        if (!widget) return;
        widget->setSaveCallback([this, checkProject, saveFunc, successMsg](const auto& data) {
            if (!checkProject()) return;
            if (saveFunc(currentProject.projectPath, data)) {
                showCenteredMessageBox(this, QMessageBox::Information, "保存成功", successMsg + "更新成功！");
            } else {
                showCenteredMessageBox(this, QMessageBox::Warning, "保存失败", successMsg + "保存失败，请检查文件权限。");
            }
        });

        connect(widget, &BaseParamWidget::backClicked, this, [this](){
            stackedWidget->setCurrentIndex(0);
            treeWidget->clearSelection();
        });
    };

    bindWidget(kneadingExplosiveWidget,  &ProjectManager::saveNieheExplosive,  "捏合-炸药参数");
    bindWidget(kneadingBladeWidget,      &ProjectManager::saveNieheParameters, "捏合-桨叶参数");
    bindWidget(kneadingBoundaryWidget,   &ProjectManager::saveNieheBoundary,   "捏合-边界条件");
    bindWidget(kneadingSimulationWidget, &ProjectManager::saveNieheSimulation, "捏合-仿真设置");

    bindWidget(extrusionExplosiveWidget, &ProjectManager::saveExtrusionExplosive, "挤压-炸药参数");
    bindWidget(extrusionScrewWidget,     &ProjectManager::saveExtrusionScrew,     "挤压-螺杆参数");
    bindWidget(extrusionBoundaryWidget,  &ProjectManager::saveExtrusionBoundary,  "挤压-边界条件");
    bindWidget(extrusionSimulationWidget,&ProjectManager::saveExtrusionSimulation, "挤压-仿真设置");

    connect(infoWidget, &BaseParamWidget::backClicked, this, [this](){
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });
}

void MainWindow::createPureStyleToolBar()
{
    QToolBar *oldBar = findChild<QToolBar*>();
    if(oldBar) delete oldBar;

    QToolBar *toolBar = addToolBar(tr("工具栏"));
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

    addBtn("新建工程", ":/new/prefix1/toolbar_picture/create.png", &MainWindow::newProject, false);
    addBtn("打开工程", ":/new/prefix1/toolbar_picture/open.png", &MainWindow::openProject, false);

    QAction *saveAct = new QAction(QIcon(":/new/prefix1/toolbar_picture/save.png"), "保存工程", this);
    connect(saveAct, &QAction::triggered, this, [this](){ this->saveProject(false); });
    toolBar->addAction(saveAct);
    projectDependentActions.append(saveAct);

    addBtn("关闭工程", ":/new/prefix1/toolbar_picture/close.png", &MainWindow::exitProject);

    toolBar->addSeparator();

    addBtn("工程信息", ":/new/prefix1/toolbar_picture/information.png", &MainWindow::projectInfo);
    addBtn("炸药参数", ":/new/prefix1/toolbar_picture/cailiaocanshu.png", &MainWindow::explosiveParams);
    addBtn("结构参数", ":/new/prefix1/toolbar_picture/jiegoucanshu.png", &MainWindow::bladeParams);
    addBtn("边界条件", ":/new/prefix1/toolbar_picture/fangzhenshezhi.png", &MainWindow::boundaryConditions);
    addBtn("仿真设置", ":/new/prefix1/toolbar_picture/fangzhenshezhi.png", &MainWindow::simulationSettings);

    toolBar->addSeparator();

    addBtn("参数检查", ":/new/prefix1/toolbar_picture/check.png", &MainWindow::checkParams);
    
    // 【修改开始】显式创建“生成文件”动作，以便后续控制状态
    generateAction = new QAction(QIcon(":/new/prefix1/toolbar_picture/file.png"), "生成文件", this);
    connect(generateAction, &QAction::triggered, this, &MainWindow::generateFiles);
    toolBar->addAction(generateAction);
    projectDependentActions.append(generateAction);
    // 【修改结束】

    startAction = new QAction(QIcon(":/new/prefix1/toolbar_picture/start.png"), "开始仿真", this);
    connect(startAction, &QAction::triggered, this, &MainWindow::startSimulation);
    toolBar->addAction(startAction);
    projectDependentActions.append(startAction);

    addBtn("生成报告", ":/new/prefix1/toolbar_picture/report.png", &MainWindow::generateReport);



    toolBar->addSeparator();

    addBtn("系统设置", "://new/prefix1/toolbar_picture/setup.png", &MainWindow::settings, false);
    addBtn("帮助文档", "://new/prefix1/toolbar_picture/help.png", &MainWindow::help, false);

    QAction *closeAppAct = new QAction(QIcon(":/new/prefix1/toolbar_picture/closeall.png"), "关闭", this);
    if(closeAppAct->icon().isNull()) closeAppAct->setIcon(QIcon(":/new/prefix1/toolbar_picture/close.png"));
    connect(closeAppAct, &QAction::triggered, this, &MainWindow::close);
    toolBar->addAction(closeAppAct);
}

void MainWindow::createTreeWidget() {
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

    QStringList categories;
    categories << "捏合混匀工艺 (Kneading)" << "挤压造粒工艺 (Extrusion)" << "压制成型工艺 (Pressing)";
    for (const QString &catName : categories) {
        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, catName);
        QFont font = item->font(0);
        font.setBold(true);
        font.setPointSize(13);
        item->setFont(0, font);
        item->setExpanded(true);
        item->setData(0, Qt::UserRole, "CATEGORY_ROOT");
        item->setForeground(0, QBrush(QColor("#333")));
    }
}

void MainWindow::updateTreeStructure(const QString &name, int type, const QString &path) {
    if (!treeWidget) return;
    if (type < 0 || type >= treeWidget->topLevelItemCount()) return;
    QTreeWidgetItem *root = treeWidget->topLevelItem(type);
    for (int i = 0; i < root->childCount(); ++i) {
        if (root->child(i)->text(0) == name && root->child(i)->data(0, Qt::UserRole).toString() == path) {
            treeWidget->setCurrentItem(root->child(i));
            root->setExpanded(true);
            return;
        }
    }
    QTreeWidgetItem *projectItem = new QTreeWidgetItem(root);
    projectItem->setText(0, name);
    projectItem->setIcon(0, QIcon(":/new/prefix1/toolbar_picture/file.png"));
    projectItem->setData(0, Qt::UserRole, path);
    QFont font = projectItem->font(0);
    font.setBold(true);
    font.setPointSize(13);
    projectItem->setFont(0, font);
    projectItem->setExpanded(true);

    QStringList steps;
    steps << "工程信息" << "炸药参数";
    if (type == 0) {
        steps << "桨叶参数";
    } else if (type == 1) {
        steps << "运动部件";
    } else if (type == 2) {
        steps << "模具参数";
    }
    steps << "边界条件" << "仿真设置" << "参数检查" << "生成文件" << "开始仿真" << "生成报告";

    for (const QString &s : steps) {
        QTreeWidgetItem *child = new QTreeWidgetItem(projectItem);
        child->setText(0, s);
        child->setData(0, Qt::UserRole, path);
        QFont cFont = child->font(0);
        cFont.setPointSize(13);
        child->setFont(0, cFont);
        QString icon;
        if (s == "工程信息") icon = "information";
        else if (s == "炸药参数") icon = "cailiaocanshu";
        else if (s.contains("参数") || s.contains("部件")) icon = "jiegoucanshu";
        else if (s == "边界条件" || s == "仿真设置") icon = "fangzhenshezhi";
        else if (s == "参数检查") icon = "check";
        else if (s == "生成文件") icon = "file";
        else if (s == "开始仿真") icon = "start";
        else if (s == "生成报告") icon = "report";

        if(!icon.isEmpty()) {
            child->setIcon(0, QIcon(":/new/prefix1/toolbar_picture/" + icon + ".png"));
        }
    }
    treeWidget->setCurrentItem(projectItem);
    root->setExpanded(true);
}

void MainWindow::selectTreeItem(const QString &itemName) {
    if (!treeWidget) return;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *cat = treeWidget->topLevelItem(i);
        for (int j = 0; j < cat->childCount(); ++j) {
            QTreeWidgetItem *proj = cat->child(j);
            if (proj->data(0, Qt::UserRole).toString() == currentProject.projectPath) {
                for (int k = 0; k < proj->childCount(); ++k) {
                    if (proj->child(k)->text(0) == itemName) {
                        treeWidget->setCurrentItem(proj->child(k));
                        treeWidget->scrollToItem(proj->child(k));
                        return;
                    }
                }
            }
        }
    }
}

void MainWindow::newProject() {
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getProjectName();
        QString path = dialog.getProjectPath();
        QString fullPath = path + "/" + name;
        int type = dialog.getProcessType();

        QDir dir(fullPath);
        if (dir.exists()) { showCenteredMessageBox(this, QMessageBox::Warning, "错误", "工程已存在"); return; }

        if (dir.mkpath(".")) {
            currentProject = {name, fullPath, type, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), "1.0"};
            isProjectLoaded = true;
            bool ok = ProjectManager::saveProject(fullPath, currentProject);

            QString templateSrc;
            QString templateDst;
            QString appDir = QCoreApplication::applicationDirPath();

            auto findModelPath = [&](const QString& folderName) -> QString {
                QString path1 = appDir + "/Simulate_Models/" + folderName;
                if (QFileInfo::exists(path1) && QFileInfo(path1).isDir()) return path1;
                QString path2 = QDir::cleanPath(appDir + "/../../Simulate_Models/" + folderName);
                if (QFileInfo::exists(path2) && QFileInfo(path2).isDir()) return path2;
                return "";
            };

            if (type == 0) {
                QString found = findModelPath("25L");
                templateSrc = found.isEmpty() ? (appDir + "/Simulate_Models/25L") : found;
                templateDst = QDir(fullPath).filePath("25L");
            } else if (type == 1) {
                QString found = findModelPath("jiya");
                templateSrc = found.isEmpty() ? (appDir + "/Simulate_Models/jiya") : found;
                templateDst = QDir(fullPath).filePath("jiya");
            }

            if (!templateSrc.isEmpty()) {
                if (!ProjectManager::copyDir(templateSrc, templateDst, true)) {
                    showCenteredMessageBox(this, QMessageBox::Warning, "警告", "模型母本复制失败，请检查路径权限：\n" + templateSrc);
                }
            }

            if(type==1) {
                ProjectManager::saveExtrusionExplosive(fullPath, ProjectManager::getDefaultExtrusionExplosive());
                ProjectManager::saveExtrusionScrew(fullPath, ProjectManager::getDefaultExtrusionScrew());
                ProjectManager::saveExtrusionBoundary(fullPath, ProjectManager::getDefaultExtrusionBoundary());
                ProjectManager::saveExtrusionSimulation(fullPath, ProjectManager::getDefaultExtrusionSimulation());

                ExtrusionExplosiveData explosiveData;
                ProjectManager::loadExtrusionExplosive(fullPath, explosiveData);
                extrusionExplosiveWidget->setData(explosiveData);

                ExtrusionScrewData screwData;
                ProjectManager::loadExtrusionScrew(fullPath, screwData);
                extrusionScrewWidget->setData(screwData);

                ExtrusionBoundaryData boundaryData;
                ProjectManager::loadExtrusionBoundary(fullPath, boundaryData);
                extrusionBoundaryWidget->setData(boundaryData);

                ExtrusionSimulationData simulationData;
                ProjectManager::loadExtrusionSimulation(fullPath, simulationData);
                extrusionSimulationWidget->setData(simulationData);
            } else {
                ProjectManager::saveNieheExplosive(fullPath, ProjectManager::getDefaultNieheExplosive());
                ProjectManager::saveNieheParameters(fullPath, ProjectManager::getDefaultNieheParameters());
                ProjectManager::saveNieheBoundary(fullPath, ProjectManager::getDefaultNieheBoundary());
                ProjectManager::saveNieheSimulation(fullPath, ProjectManager::getDefaultNieheSimulation());

                NieheExplosiveData explosiveData;
                ProjectManager::loadNieheExplosive(fullPath, explosiveData);
                kneadingExplosiveWidget->setData(explosiveData);

                NieheParameters parametersData;
                ProjectManager::loadNieheParameters(fullPath, parametersData);
                kneadingBladeWidget->setData(parametersData);

                NieheBoundaryData boundaryData;
                ProjectManager::loadNieheBoundary(fullPath, boundaryData);
                kneadingBoundaryWidget->setData(boundaryData);

                NieheSimulationData simulationData;
                ProjectManager::loadNieheSimulation(fullPath, simulationData);
                kneadingSimulationWidget->setData(simulationData);
            }
            if(ok) {
                updateTreeStructure(name, type, fullPath);
                startWatchingProject();

                showCenteredMessageBox(this, QMessageBox::Information, "成功", "工程创建成功");
                updateUIStates();
            }
        }
    }
}

void MainWindow::openProject() {
    OpenProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        for(const QString &p : dialog.getSelectedPaths()) {
            ProjectConfig c;
            if(ProjectManager::loadProject(p, c)) {
                updateTreeStructure(c.projectName, c.processType, c.projectPath);
                currentProject = c;
                isProjectLoaded = true;
            }
        }
        if(isProjectLoaded) {
            startWatchingProject();
            projectInfo();
            setWindowTitle("仿真 - " + currentProject.projectName);
            loadAllProjectData(currentProject);
            updateUIStates();
        }
    }
}

void MainWindow::saveProject(bool silent) {
    if (!isProjectLoaded) return;

    infoWidget->getProjectData(currentProject);
    bool success = ProjectManager::saveProject(currentProject.projectPath, currentProject);

    if (currentProject.processType == 0) {
        NieheProjectData d;
        kneadingExplosiveWidget->getData(d.explosive);
        kneadingBladeWidget->getData(d.parameters);
        kneadingBoundaryWidget->getData(d.boundary);
        kneadingSimulationWidget->getData(d.simulation);

        success &= ProjectManager::saveNieheExplosive(currentProject.projectPath, d.explosive);
        success &= ProjectManager::saveNieheParameters(currentProject.projectPath, d.parameters);
        success &= ProjectManager::saveNieheBoundary(currentProject.projectPath, d.boundary);
        success &= ProjectManager::saveNieheSimulation(currentProject.projectPath, d.simulation);
    } else if (currentProject.processType == 1) {
        JiyaProjectData d;
        extrusionExplosiveWidget->getData(d.explosive);
        extrusionScrewWidget->getData(d.screw);
        extrusionBoundaryWidget->getData(d.boundary);
        extrusionSimulationWidget->getData(d.simulation);

        success &= ProjectManager::saveExtrusionExplosive(currentProject.projectPath, d.explosive);
        success &= ProjectManager::saveExtrusionScrew(currentProject.projectPath, d.screw);
        success &= ProjectManager::saveExtrusionBoundary(currentProject.projectPath, d.boundary);
        success &= ProjectManager::saveExtrusionSimulation(currentProject.projectPath, d.simulation);
    }

    if (!silent) {
        if (success)
            showCenteredMessageBox(this, QMessageBox::Information, "成功", "工程及所有模块参数已原子化存入磁盘。");
        else
            showCenteredMessageBox(this, QMessageBox::Critical, "错误", "部分模块保存失败，请检查工程目录权限！");
    }
}

void MainWindow::exitProject() {
    if(!treeWidget->currentItem()) return;
    QTreeWidgetItem *p = treeWidget->currentItem();
    if(p->parent() && p->parent()->parent()) p = p->parent();
    if(!p->parent()) return;

    if(showCenteredMessageBox(this, QMessageBox::Question, "关闭", "确认关闭工程 \"" + p->text(0) + "\"？", QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        QString closingPath = p->data(0, Qt::UserRole).toString();
        bool isClosingActive = (closingPath == currentProject.projectPath);
        delete p;

        bool hasAnyProject = false;
        QTreeWidgetItem *nextProject = nullptr;
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *category = treeWidget->topLevelItem(i);
            if (category->childCount() > 0) {
                hasAnyProject = true;
                if (!nextProject) nextProject = category->child(0);
            }
        }
        if (hasAnyProject && isClosingActive) {
            if(nextProject) {
                treeWidget->setCurrentItem(nextProject);
                onTreeItemClicked(nextProject, 0);
            }
        } else if (!hasAnyProject) {
            stopWatchingProject();
            isProjectLoaded = false;
            currentProject = ProjectConfig();
            stackedWidget->setCurrentIndex(0);
            setWindowTitle("仿真工程管理系统");
            updateUIStates();
            treeWidget->clearSelection();
        }
    }
}

void MainWindow::projectInfo() {
    if (!isProjectLoaded) { showCenteredMessageBox(this, QMessageBox::Warning, "提示", "请先新建或打开一个工程！"); return; }
    selectTreeItem("工程信息");
    infoWidget->setProjectData(currentProject);
    stackedWidget->setCurrentIndex(1);
}

void MainWindow::explosiveParams() {
    if (!isProjectLoaded) { showCenteredMessageBox(this, QMessageBox::Warning, "提示", "请先加载工程"); return; }
    selectTreeItem("炸药参数");
    if (currentProject.processType == 0) {
        NieheExplosiveData d;
        ProjectManager::loadNieheExplosive(currentProject.projectPath, d);
        kneadingExplosiveWidget->setData(d);
        stackedWidget->setCurrentWidget(kneadingExplosiveWidget);
    } else if (currentProject.processType == 1) {
        ExtrusionExplosiveData d;
        ProjectManager::loadExtrusionExplosive(currentProject.projectPath, d);
        extrusionExplosiveWidget->setData(d);
        stackedWidget->setCurrentWidget(extrusionExplosiveWidget);
    }
}

void MainWindow::bladeParams() {
    if (!isProjectLoaded) { showCenteredMessageBox(this, QMessageBox::Warning, "提示", "请先加载工程"); return; }
    selectTreeItem(currentProject.processType == 1 ? "运动部件" : "桨叶参数");
    if (currentProject.processType == 0) {
        NieheParameters d;
        ProjectManager::loadNieheParameters(currentProject.projectPath, d);
        kneadingBladeWidget->setData(d);
        stackedWidget->setCurrentWidget(kneadingBladeWidget);
    } else if (currentProject.processType == 1) {
        ExtrusionScrewData d;
        ProjectManager::loadExtrusionScrew(currentProject.projectPath, d);
        extrusionScrewWidget->setData(d);
        stackedWidget->setCurrentWidget(extrusionScrewWidget);
    }
}

void MainWindow::boundaryConditions() {
    if (!isProjectLoaded) { showCenteredMessageBox(this, QMessageBox::Warning, "提示", "请先加载工程"); return; }
    selectTreeItem("边界条件");
    if (currentProject.processType == 0) {
        NieheBoundaryData d;
        ProjectManager::loadNieheBoundary(currentProject.projectPath, d);
        kneadingBoundaryWidget->setData(d);
        stackedWidget->setCurrentWidget(kneadingBoundaryWidget);
    } else if (currentProject.processType == 1) {
        ExtrusionBoundaryData d;
        ProjectManager::loadExtrusionBoundary(currentProject.projectPath, d);
        extrusionBoundaryWidget->setData(d);
        stackedWidget->setCurrentWidget(extrusionBoundaryWidget);
    }
}

void MainWindow::simulationSettings() {
    if (!isProjectLoaded) { showCenteredMessageBox(this, QMessageBox::Warning, "提示", "请先加载工程"); return; }
    selectTreeItem("仿真设置");
    if (currentProject.processType == 0) {
        NieheSimulationData d;
        ProjectManager::loadNieheSimulation(currentProject.projectPath, d);
        kneadingSimulationWidget->setData(d);
        stackedWidget->setCurrentWidget(kneadingSimulationWidget);
    } else if (currentProject.processType == 1) {
        ExtrusionSimulationData d;
        ProjectManager::loadExtrusionSimulation(currentProject.projectPath, d);
        extrusionSimulationWidget->setData(d);
        stackedWidget->setCurrentWidget(extrusionSimulationWidget);
    }
}


void MainWindow::checkParams() {
    selectTreeItem("参数检查");
    if (!isProjectLoaded) { showCenteredMessageBox(this, QMessageBox::Warning, "警告", "请先新建或打开一个工程！"); return; }

    QDialog dlg(this);
    dlg.setWindowTitle("参数完整性检查");
    dlg.resize(800, 850);
    dlg.setMinimumSize(800, 600);
    dlg.setStyleSheet("background-color: white;");

    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
    dlgLayout->setContentsMargins(0,0,0,0);

    QScrollArea *scrollArea = new QScrollArea(&dlg);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background-color: white;");

    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(5);

    auto renderParams = [&](BaseParamWidget* widget, const QString& groupTitle) {
        if(!widget) return;
        QGroupBox *box = createSectionBox(groupTitle);
        QVBoxLayout *boxLayout = (QVBoxLayout*)box->layout();

        QList<ParamInfo> params = widget->getRegisteredParams();
        if (params.isEmpty()) {
            QLabel *l = new QLabel("（无参数）");
            l->setStyleSheet("color:red; font-size:16px; font-family: 'Microsoft YaHei';");
            boxLayout->addWidget(l);
        }

        for(const ParamInfo &info : params) {
            if (info.isTitle) {
                QLabel *lblSub = new QLabel(info.label);
                lblSub->setAlignment(Qt::AlignCenter);
                lblSub->setMinimumHeight(100);
                lblSub->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
                lblSub->setStyleSheet("font-family: 'Microsoft YaHei'; color: #000; font-weight: bold; font-size: 20px; margin-top: 10px; margin-bottom: 3px; border-bottom: 1px solid #eee; padding-bottom: 3px;");
                boxLayout->addWidget(lblSub);
            } else {
                QString val = info.edit ? info.edit->text() : "";
                boxLayout->addLayout(createInfoRow(info.label + "：", val));
            }
        }
        contentLayout->addWidget(box);
    };

    if (currentProject.processType == 0) {
        renderParams(kneadingExplosiveWidget, "1. 炸药材料参数");
        renderParams(kneadingBladeWidget,     "2. 桨叶结构参数");
        renderParams(kneadingBoundaryWidget,  "3. 边界条件");
        renderParams(kneadingSimulationWidget,"4. 仿真控制");
    } else {
        renderParams(extrusionExplosiveWidget, "1. 炸药材料参数");
        renderParams(extrusionScrewWidget,     "2. 螺杆结构参数");
        renderParams(extrusionBoundaryWidget,  "3. 边界条件");
        renderParams(extrusionSimulationWidget,"4. 仿真控制");
    }

    contentLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    dlgLayout->addWidget(scrollArea);

    QWidget *bottomWidget = new QWidget(&dlg);
    bottomWidget->setStyleSheet("background-color: white; border-top: 1px solid #eee;");
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomWidget);
    QPushButton *btnOk = new QPushButton("确认无误", bottomWidget);
    btnOk->setFixedSize(140, 45);
    btnOk->setStyleSheet("QPushButton { background-color: #1890ff; color: white; border-radius: 5px; font-weight: bold; font-size: 16px; font-family: 'Microsoft YaHei'; } QPushButton:hover { background-color: #40a9ff; }");
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnOk);
    bottomLayout->addStretch();
    dlgLayout->addWidget(bottomWidget);

    connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

// 1. 实现捏合文件生成逻辑
bool MainWindow::generateKneadingFiles() {
    NieheProjectData data;
    // 从界面获取数据
    kneadingExplosiveWidget->getData(data.explosive);
    kneadingBladeWidget->getData(data.parameters);
    kneadingBoundaryWidget->getData(data.boundary);
    kneadingSimulationWidget->getData(data.simulation);
    
    // 调用 Manager 生成文件
    return ProjectManager::generateNiehePolyflowFile(currentProject.projectPath, data);
}

// 2. 实现挤压文件生成逻辑
bool MainWindow::generateExtrusionFiles() {
    JiyaProjectData data;
    // 从界面获取数据
    extrusionExplosiveWidget->getData(data.explosive);
    extrusionScrewWidget->getData(data.screw);
    extrusionBoundaryWidget->getData(data.boundary);
    extrusionSimulationWidget->getData(data.simulation);
    
    // 调用 Manager 生成文件
    return ProjectManager::generateJiyaPolyflowFile(currentProject.projectPath, data);
}

// 1. 实现捏合仿真启动逻辑
void MainWindow::startKneadingSimulation() {
    // 1. 获取捏合工艺的仿真目录 (需与 SimulationManager 中的路径逻辑保持一致)
    QDir projectDir(currentProject.projectPath);
    QString workDir = projectDir.filePath("25L/Simulation/mix");
    QString datFile = QDir(workDir).filePath("polyflow.dat");

    // 2. 检查 polyflow.dat 是否存在
    if (!QFileInfo::exists(datFile)) {
        // 使用 StandardButton 捕获用户的选择
        QMessageBox::StandardButton btn = showCenteredMessageBox(this, QMessageBox::Warning, "文件缺失",
            "检测到当前目录下缺少仿真定义文件 (polyflow.dat)。\n\n"
            "您必须先执行\"生成文件\"操作才能开始仿真。\n"
            "是否立即为您生成？",
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

        if (btn == QMessageBox::Yes) {
            // 用户点击了“是”，尝试自动生成
            if (generateKneadingFiles()) {
                // 如果生成成功，递归调用自己，再次尝试启动
                // (此时文件已存在，会直接进入步骤 3)
                startKneadingSimulation();
            }
        }
        // 如果用户点取消或生成失败，直接返回，不启动仿真
        return;
    }

    // 3. 一切就绪，文件存在，通过管理器启动任务
    SimulationManager::instance().startTask(currentProject.projectPath, currentProject);
}

// 2. 实现挤压仿真启动逻辑
void MainWindow::startExtrusionSimulation() {
    // 1. 获取挤压工艺的仿真目录 (注意路径区别)
    QDir projectDir(currentProject.projectPath);
    QString workDir = projectDir.filePath("jiya/Simulation/jiya");
    QString datFile = QDir(workDir).filePath("polyflow.dat");

    // 2. 检查 polyflow.dat 是否存在
    if (!QFileInfo::exists(datFile)) {
        QMessageBox::StandardButton btn = showCenteredMessageBox(this, QMessageBox::Warning, "文件缺失",
            "检测到当前目录下缺少仿真定义文件 (polyflow.dat)。\n\n"
            "您必须先执行\"生成文件\"操作才能开始仿真。\n"
            "是否立即为您生成？",
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

        if (btn == QMessageBox::Yes) {
            // 尝试自动生成
            if (generateExtrusionFiles()) {
                // 生成成功，再次尝试启动
                startExtrusionSimulation();
            }
        }
        return;
    }

    // 3. 一切就绪，通过管理器启动任务
    SimulationManager::instance().startTask(currentProject.projectPath, currentProject);
}

void MainWindow::generateFiles() {
    selectTreeItem("生成文件");

    // --- 通用检查 ---
    if (!isProjectLoaded) {
        showCenteredMessageBox(this, QMessageBox::Warning, "警告", "请先新建或打开一个工程！");
        return;
    }

    if (SimulationManager::instance().isRunning(currentProject.projectPath)) {
        showCenteredMessageBox(this, QMessageBox::Warning, "操作受限", "仿真正在运行中，无法覆盖文件，请先终止仿真。");
        return;
    }

    // --- 通用确认 ---
    QMessageBox::StandardButton btn = showCenteredMessageBox(this, QMessageBox::Question, "确认生成",
        "确定要重新生成仿真文件吗？\n当前目录下的 polyflow.dat 将被覆盖。",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (btn != QMessageBox::Yes) return;

    // --- 保存当前参数 ---
    saveProject(true);

    // --- 分流处理 ---
    bool success = false;
    if (currentProject.processType == 0) {
        // 捏合工艺
        success = generateKneadingFiles();
    } else if (currentProject.processType == 1) {
        // 挤压工艺
        success = generateExtrusionFiles();
    }

    // --- 结果反馈 ---
    if (success) {
        showCenteredMessageBox(this, QMessageBox::Information, "成功", "文件已生成。");
    } else {
        showCenteredMessageBox(this, QMessageBox::Critical, "失败", "文件生成失败，请检查目录权限。");
    }
}

// 3. 重构主 startSimulation 函数
void MainWindow::startSimulation() {
    selectTreeItem("开始仿真");
    if (!isProjectLoaded) return;

    // --- 运行状态检查 ---
    if (SimulationManager::instance().isRunning(currentProject.projectPath)) {
        stackedWidget->setCurrentWidget(simulationMonitorWidget);
        return;
    }

    // --- 启动确认 ---
    QMessageBox::StandardButton btn = showCenteredMessageBox(this, QMessageBox::Question, "启动仿真确认",
        QString("您即将启动项目 \"%1\" 的仿真计算。\n\n"
                "请确认以下事项：\n"
                "1. 所有参数（炸药、桨叶、边界等）已设置无误。\n"
                "2. 上一次计算的结果文件（如有）将会被覆盖。\n"
                "3. 仿真过程可能占用较多 CPU 资源。\n\n"
                "确定要立即开始吗？").arg(currentProject.projectName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);


    if (btn != QMessageBox::Yes) return;

    // 3. 用户确认后，根据工艺类型调用不同的启动函数
    if (currentProject.processType == 0) {
        // 捏合工艺
        startKneadingSimulation();
    } else if (currentProject.processType == 1) {
        // 挤压工艺
        startExtrusionSimulation();
    }

    // 4. 跳转到监控页面
    stackedWidget->setCurrentWidget(simulationMonitorWidget);
    
    // 5. 【关键】立即刷新按钮状态 (让生成文件按钮变灰)
    updateSimulationActionStates();
}

void MainWindow::terminateSimulation()
{
    if (showCenteredMessageBox(this, QMessageBox::Question, "确认终止", "确定要终止当前项目的仿真吗？", QMessageBox::Yes|QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    // 【核心替换】调用 stopTask
    SimulationManager::instance().stopTask(currentProject.projectPath);
    updateSimulationActionStates();
}

void MainWindow::generateReport() {
    selectTreeItem("生成报告");
    showCenteredMessageBox(this, QMessageBox::Information, "演示", "报告生成中");
}

void MainWindow::settings()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString path = SettingsDialog::getPolyflowPath();
    }
}

void MainWindow::help()
{
    // 帮助文档功能实现
    showCenteredMessageBox(this, QMessageBox::Information, "帮助", "帮助文档功能正在开发中...");
}

QMessageBox::StandardButton MainWindow::showCenteredMessageBox(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
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
    if (showCenteredMessageBox(this, QMessageBox::Question, "退出", "确定要退出吗？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}



void MainWindow::onTreeItemDoubleClicked(QTreeWidgetItem *item, int c) {
    Q_UNUSED(c);
    onTreeItemClicked(item, c);
    if(item->parent()&&!item->parent()->parent()) projectInfo();
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem *item, int c) {
    Q_UNUSED(c);
    if(!item->parent() || item->isDisabled()) return;

    QString path = item->data(0, Qt::UserRole).toString();

    // 1. 工程切换逻辑
    if(!path.isEmpty() && path != currentProject.projectPath) {
        ProjectConfig c;
        if(ProjectManager::loadProject(path, c)) {
            currentProject = c;
            isProjectLoaded = true;
            setWindowTitle("仿真 - "+c.projectName);
            loadAllProjectData(c);
            
            // ===============================================
            // 【核心修复】切换项目时，恢复视图状态
            // ===============================================
            
            // 1. 先清空当前显示
            logOutput->clear();
            
            // 2. 检查该项目是否正在运行
            if (SimulationManager::instance().isRunning(currentProject.projectPath)) {
                // 如果在运行，从 Manager 获取“记忆”
                QString savedLog = SimulationManager::instance().getProjectLog(currentProject.projectPath);
                int savedProgress = SimulationManager::instance().getProjectProgress(currentProject.projectPath);
                QString savedStatus = SimulationManager::instance().getProjectStatus(currentProject.projectPath);

                // 恢复显示
                logOutput->setPlainText(savedLog);
                logOutput->moveCursor(QTextCursor::End); // 滚动到底部
                progressBar->setValue(savedProgress);
                statusLabel->setText(savedStatus);
            } else {
                // 如果没运行，重置为初始状态
                logOutput->setPlaceholderText("等待仿真启动...");
                progressBar->setValue(0);
                statusLabel->setText("准备就绪");
            }
            
            updateUIStates();
        } else {
            showCenteredMessageBox(this, QMessageBox::Warning, "错误", "该工程文件夹已不存在或损坏，将从列表中移除。");
            delete item;
            return;
        }
    }

    // 2. 页面跳转
    QString txt = item->text(0);
    static const QMap<QString, void(MainWindow::*)()> actionMap = {
        {"工程信息", &MainWindow::projectInfo},
        {"炸药参数", &MainWindow::explosiveParams},
        {"桨叶参数", &MainWindow::bladeParams},
        {"运动部件", &MainWindow::bladeParams},
        {"边界条件", &MainWindow::boundaryConditions},
        {"仿真设置", &MainWindow::simulationSettings},
        {"参数检查", &MainWindow::checkParams},
        {"生成文件", &MainWindow::generateFiles},
        {"开始仿真", &MainWindow::startSimulation},
        {"生成报告", &MainWindow::generateReport}
    };

    if (actionMap.contains(txt)) {
        (this->*actionMap[txt])();
    }
}

void MainWindow::startWatchingProject() {
    if (!fileWatcher) return;
    if (!fileWatcher->directories().isEmpty()) fileWatcher->removePaths(fileWatcher->directories());

    QSet<QString> uniquePaths;
    QTreeWidgetItemIterator it(treeWidget);
    while (*it) {
        if ((*it)->parent() && !(*it)->parent()->parent()) {
            QString path = (*it)->data(0, Qt::UserRole).toString();
            if (!path.isEmpty()) {
                QFileInfo info(path);
                QString parentPath = info.absolutePath();
                if (QFileInfo(parentPath).exists()) uniquePaths.insert(parentPath);
                if (info.exists()) uniquePaths.insert(path);
            }
        }
        ++it;
    }
    if (!uniquePaths.isEmpty()) fileWatcher->addPaths(uniquePaths.values());
}

void MainWindow::stopWatchingProject()
{
    if(fileWatcher) fileWatcher->removePaths(fileWatcher->directories());
}

void MainWindow::onProjectDirectoryChanged() {
    QList<QTreeWidgetItem*> itemsToDelete;
    bool currentProjectDeleted = false;

    for(int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *group = treeWidget->topLevelItem(i);
        for(int j = 0; j < group->childCount(); ++j) {
            QTreeWidgetItem *projectItem = group->child(j);
            QString p = projectItem->data(0, Qt::UserRole).toString();

            if(!p.isEmpty() && !QFileInfo(p).exists()) {
                itemsToDelete.append(projectItem);
                if(isProjectLoaded && p == currentProject.projectPath) currentProjectDeleted = true;
            }
        }
    }

    for(auto item : itemsToDelete) delete item;

    if(currentProjectDeleted) {
        stopWatchingProject();
        isProjectLoaded = false;
        currentProject = ProjectConfig();
        stackedWidget->setCurrentIndex(0);
        setWindowTitle("仿真工程管理系统");
        updateUIStates();
        startWatchingProject();
    }
}

void MainWindow::updateUIStates() {
    for (QAction *act : projectDependentActions) act->setEnabled(isProjectLoaded);
    updateSimulationActionStates();
}

void MainWindow::updateSimulationActionStates() {
    // 1. 获取当前项目的运行状态
    bool isRunning = false;
    if (isProjectLoaded) {
        isRunning = SimulationManager::instance().isRunning(currentProject.projectPath);
    }

    // 2. 控制工具栏按钮状态
    if (generateAction) {
        // 只有在“项目已加载”且“未运行”时，才能生成文件
        generateAction->setEnabled(isProjectLoaded && !isRunning);
    }
    if (startAction) {
        startAction->setEnabled(isProjectLoaded);
    }

    // 3. 【高级优化】同时控制左侧树形菜单的“生成文件”节点
    if (treeWidget) {
        // 查找所有名为“生成文件”的节点
        QList<QTreeWidgetItem*> items = treeWidget->findItems("生成文件", Qt::MatchRecursive);
        for (QTreeWidgetItem* item : items) {
            // 获取该节点所属的工程路径
            QString path = item->data(0, Qt::UserRole).toString();
            
            // 检查该工程是否正在运行
            bool itemRunning = SimulationManager::instance().isRunning(path);
            
            // 如果正在运行，禁用该节点（变灰，不可点击）；否则启用
            item->setDisabled(itemRunning);
        }
    }
}

// 响应SimulationManager信号
void MainWindow::onSimulationLogReceived(QString projectPath, QString message) {
    if (projectPath == currentProject.projectPath) {
        logOutput->moveCursor(QTextCursor::End);
        logOutput->insertPlainText(message);
        logOutput->moveCursor(QTextCursor::End);
    }
}

void MainWindow::onSimulationProgress(QString projectPath, int percent, QString status) {
    if (projectPath == currentProject.projectPath) {
        progressBar->setValue(percent);
        statusLabel->setText(status);
    }
}

void MainWindow::onSimulationFinished(QString projectPath, int exitCode) {
    if (projectPath == currentProject.projectPath) {
        if (exitCode == 0) {
            progressBar->setValue(100);
            statusLabel->setText("计算完成");
            logOutput->append("\n>>> 仿真计算成功完成！\n");
            QMessageBox msgBox(QMessageBox::Information, "完成", "仿真计算已结束！\n请检查项目目录下的结果文件。", QMessageBox::Ok, this);
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.exec();
        }
        else if (exitCode == -1) {
            // 【核心优化】用户手动终止 (蓝色提示)
            progressBar->setValue(0);
            statusLabel->setText("已停止");
            logOutput->append("\n>>> 仿真已由用户手动终止。\n");
            QMessageBox msgBox(QMessageBox::Information, "提示", "仿真任务已成功停止。", QMessageBox::Ok, this);
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.exec();
        }
        else {
            statusLabel->setText("计算失败");
            logOutput->append(QString("\n>>> 仿真异常结束 (错误码: %1)\n").arg(exitCode));
            QMessageBox msgBox(QMessageBox::Warning, "错误", QString("仿真引擎异常退出！\n错误码: %1\n可能原因：参数设置错误或求解器崩溃。").arg(exitCode), QMessageBox::Ok, this);
            msgBox.setWindowModality(Qt::ApplicationModal);
            msgBox.exec();
        }
        updateSimulationActionStates();
    }
}

void MainWindow::loadAllProjectData(const ProjectConfig &config) {
    bool allLoadedSuccessfully = true;

    if (config.processType == 0) {
        NieheExplosiveData ed;
        if (!ProjectManager::loadNieheExplosive(config.projectPath, ed)) allLoadedSuccessfully = false;
        kneadingExplosiveWidget->setData(ed);

        NieheBoundaryData bd;
        if (!ProjectManager::loadNieheBoundary(config.projectPath, bd)) allLoadedSuccessfully = false;
        kneadingBoundaryWidget->setData(bd);

        NieheParameters mp;
        if (!ProjectManager::loadNieheParameters(config.projectPath, mp)) allLoadedSuccessfully = false;
        kneadingBladeWidget->setData(mp);

        NieheSimulationData sd;
        if (!ProjectManager::loadNieheSimulation(config.projectPath, sd)) allLoadedSuccessfully = false;
        kneadingSimulationWidget->setData(sd);
    } else {
        ExtrusionExplosiveData ed;
        if (!ProjectManager::loadExtrusionExplosive(config.projectPath, ed)) allLoadedSuccessfully = false;
        extrusionExplosiveWidget->setData(ed);

        ExtrusionBoundaryData bd;
        if (!ProjectManager::loadExtrusionBoundary(config.projectPath, bd)) allLoadedSuccessfully = false;
        extrusionBoundaryWidget->setData(bd);

        ExtrusionScrewData sd;
        if (!ProjectManager::loadExtrusionScrew(config.projectPath, sd)) allLoadedSuccessfully = false;
        extrusionScrewWidget->setData(sd);

        ExtrusionSimulationData simd;
        if (!ProjectManager::loadExtrusionSimulation(config.projectPath, simd)) allLoadedSuccessfully = false;
        extrusionSimulationWidget->setData(simd);
    }

    if (!allLoadedSuccessfully) {
        showCenteredMessageBox(this, QMessageBox::Warning, "警告", "部分项目数据加载失败，可能使用了默认值。");
    }
}
