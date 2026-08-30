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
#include "ProjectInputHash.h"
#include "SimulationReportGenerator.h"
#include "SimulationResultService.h"

#include <QBrush>
#include <QCloseEvent>
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QPushButton>
#include <QResizeEvent>
#include <QSet>
#include <QSizePolicy>
#include <QTimer>
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

const QString NODE_SIMULATION_RESULT =
    QStringLiteral("SIMULATION_RESULT");

enum PreviousResultAction {
    PreviousResultCancel = 0,
    PreviousResultViewLogs,
    PreviousResultRerun
};

enum PostProcessResumeAction {
    PostProcessResumeCancel = 0,
    PostProcessResumeContinue,
    PostProcessResumeFullRun
};

PreviousResultAction promptPreviousSimulationResult(
    QWidget *parent,
    const QString &message)
{
    QMessageBox msgBox(
        QMessageBox::Question,
        QStringLiteral("上一次仿真已正常完成"),
        message,
        QMessageBox::NoButton,
        parent
    );

    QPushButton *viewLogsButton = msgBox.addButton(
        QStringLiteral("查看上次仿真日志"),
        QMessageBox::ActionRole
    );
    QPushButton *rerunButton = msgBox.addButton(
        QStringLiteral("重新进行仿真"),
        QMessageBox::AcceptRole
    );
    QPushButton *cancelButton = msgBox.addButton(
        QStringLiteral("取消"),
        QMessageBox::RejectRole
    );

    msgBox.setDefaultButton(cancelButton);
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.exec();

    QAbstractButton *clicked = msgBox.clickedButton();
    if (clicked == viewLogsButton) {
        return PreviousResultViewLogs;
    }
    if (clicked == rerunButton) {
        return PreviousResultRerun;
    }
    if (clicked == cancelButton) {
        return PreviousResultCancel;
    }

    return PreviousResultCancel;
}

PostProcessResumeAction promptPostProcessResume(
    QWidget *parent,
    const QString &message)
{
    QMessageBox msgBox(
        QMessageBox::Question,
        QStringLiteral("检测到已完成的 Abaqus 求解"),
        message
            + QStringLiteral(
                "\n\n请选择后续操作。"
            ),
        QMessageBox::NoButton,
        parent
    );

    QPushButton *continueButton =
        msgBox.addButton(
            QStringLiteral("继续后处理"),
            QMessageBox::AcceptRole
        );

    QPushButton *fullRunButton =
        msgBox.addButton(
            QStringLiteral("重新完整仿真"),
            QMessageBox::DestructiveRole
        );

    QPushButton *cancelButton =
        msgBox.addButton(
            QStringLiteral("取消"),
            QMessageBox::RejectRole
        );

    msgBox.setDefaultButton(continueButton);
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.exec();

    if (msgBox.clickedButton() == continueButton) {
        return PostProcessResumeContinue;
    }

    if (msgBox.clickedButton() == fullRunButton) {
        return PostProcessResumeFullRun;
    }

    Q_UNUSED(cancelButton);
    return PostProcessResumeCancel;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("浇注XX固化监测与三维参数重构分析软件"));
    setMinimumSize(1300, 800);
    resize(1300, 800);
    setupUi();

    homeTitleResizeTimer = new QTimer(this);
    homeTitleResizeTimer->setSingleShot(true);
    homeTitleResizeTimer->setInterval(120);
    connect(
        homeTitleResizeTimer,
        &QTimer::timeout,
        this,
        &MainWindow::updateHomeTitleFont
    );

    setStyleSheet(R"(
    QMainWindow {
        border-image: url(:/toolbar/back.png) 0 0 0 0 stretch stretch;
    }
    )");

    fileWatcher = new QFileSystemWatcher(this);
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(100);

    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() { debounceTimer->start(); });
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, [this]() { debounceTimer->start(); });
    connect(debounceTimer, &QTimer::timeout, this, &MainWindow::onProjectDirectoryChanged);

    simulationManager = new SimulationManager(this);
    connectSimulationManager();

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

    leftPaneWidget = new QWidget(centralWidget);
    leftPaneWidget->setMinimumWidth(220);
    leftPaneWidget->setMaximumWidth(600);
    leftPaneWidget->setStyleSheet(
        QStringLiteral(
            "background-color: #ffffff;"
            "border-right: 1px solid #e6e6e6;"
        )
    );

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPaneWidget);
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

    titleLabel = new QLabel(QStringLiteral("浇注XX固化监测与三维参数重构分析软件"), stackedWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setWordWrap(true);
    titleLabel->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding
    );
    titleLabel->setMinimumSize(0, 0);

    QFont titleFont(QStringLiteral("Microsoft YaHei UI"));
    titleFont.setPixelSize(136);
    titleFont.setWeight(QFont::Medium);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, 96.5);
    titleLabel->setFont(titleFont);

    titleLabel->setStyleSheet(
        QStringLiteral(
            "color: #1d2a3d;"
            "background: transparent;"
            "padding: 20px;"
        )
    );

    QGraphicsDropShadowEffect *titleShadow =
        new QGraphicsDropShadowEffect(titleLabel);
    titleShadow->setBlurRadius(6);
    titleShadow->setOffset(0, 1);
    titleShadow->setColor(QColor(18, 32, 50, 32));
    titleLabel->setGraphicsEffect(titleShadow);

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

    simulationPrepareWidget = new SimulationPrepareWidget(stackedWidget);
    stackedWidget->addWidget(simulationPrepareWidget);

    resultViewerWidget = new ResultViewerWidget(stackedWidget);
    stackedWidget->addWidget(resultViewerWidget);

    connect(
        stackedWidget,
        &QStackedWidget::currentChanged,
        this,
        [this](int) {
            if (resultViewerWidget
                && stackedWidget->currentWidget() != resultViewerWidget) {
                resultViewerWidget->stopPlayback();
            }
        }
    );

    connect(
        resultViewerWidget,
        &ResultViewerWidget::generateReportRequested,
        this,
        &MainWindow::generateSimulationReport
    );
    connect(
        resultViewerWidget,
        &ResultViewerWidget::openResultsDirectoryRequested,
        this,
        &MainWindow::openResultsDirectory
    );
    connect(
        resultViewerWidget,
        &ResultViewerWidget::continueSimulationRequested,
        this,
        &MainWindow::showSimulationPreparePage
    );

    connect(
        simulationPrepareWidget,
        &SimulationPrepareWidget::startRequested,
        this,
        &MainWindow::startSimulation
    );

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

    connectParameterEditSignals();

    connect(parameterCheckWidget, &BaseParamWidget::backClicked, this, [this]() {
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    connect(resultViewerWidget, &BaseParamWidget::backClicked, this, [this]() {
        if (resultViewerWidget) {
            resultViewerWidget->stopPlayback();
        }
        stackedWidget->setCurrentIndex(0);
        treeWidget->clearSelection();
    });

    rightLayout->addWidget(stackedWidget, 0, 0, 1, 1);

    mainLayout->addWidget(leftPaneWidget, 0);
    mainLayout->addWidget(rightWidget, 1);

    setCentralWidget(centralWidget);
    adjustProjectPaneToContents();

    QTimer::singleShot(
        0,
        this,
        [this]() {
            updateHomeTitleFont();
        }
    );
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

    auto addBtn = [&](const QString &txt, const QString &icon, void (MainWindow::*slot)(), bool needsProject = true) -> QAction * {
        QAction *act = new QAction(QIcon(icon), txt, this);
        connect(act, &QAction::triggered, this, slot);
        toolBar->addAction(act);
        if (needsProject) {
            projectDependentActions.append(act);
        }
        return act;
    };

    addBtn(QStringLiteral("新建工程"), QStringLiteral(":/toolbar/create.png"), &MainWindow::newProject, false);
    addBtn(QStringLiteral("打开工程"), QStringLiteral(":/toolbar/open.png"), &MainWindow::openProject, false);
    addBtn(QStringLiteral("关闭工程"), QStringLiteral(":/toolbar/close.png"), &MainWindow::exitProject);

    toolBar->addSeparator();

    addBtn(QStringLiteral("工程信息"), QStringLiteral(":/toolbar/information.png"), &MainWindow::projectInfo);
    addBtn(
        QStringLiteral("炸药参数"),
        QStringLiteral(":/toolbar/cailiaocanshu.png"),
        &MainWindow::explosiveParams
    );
    addBtn(
        QStringLiteral("结构参数"),
        QStringLiteral(":/toolbar/jiegoucanshu.png"),
        &MainWindow::structureParams
    );
    addBtn(
        QStringLiteral("模具参数"),
        QStringLiteral(":/toolbar/jiegoucanshu.png"),
        &MainWindow::moldParams
    );
    addBtn(
        QStringLiteral("边界条件"),
        QStringLiteral(":/toolbar/fangzhenshezhi.png"),
        &MainWindow::boundaryParams
    );
    addBtn(
        QStringLiteral("仿真设置"),
        QStringLiteral(":/toolbar/fangzhenshezhi.png"),
        &MainWindow::simulationParams
    );

    toolBar->addSeparator();

    addBtn(QStringLiteral("参数检查"), QStringLiteral(":/toolbar/check.png"), &MainWindow::checkParams);
    QAction *generateAction =
        addBtn(
            QStringLiteral("生成文件"),
            QStringLiteral(":/toolbar/file.png"),
            &MainWindow::generateFiles
        );
    simulationLockedActions << generateAction;
    addBtn(QStringLiteral("开始仿真"), QStringLiteral(":/toolbar/start.png"), &MainWindow::showSimulationPreparePage);

    stopSimulationAction = new QAction(
        QIcon(QStringLiteral(":/toolbar/close.png")),
        QStringLiteral("终止仿真"),
        this
    );
    connect(
        stopSimulationAction,
        &QAction::triggered,
        this,
        &MainWindow::stopSimulation
    );
    stopSimulationAction->setEnabled(false);
    toolBar->addAction(stopSimulationAction);

    QAction *resultAction =
        addBtn(
            QStringLiteral("仿真结果"),
            QStringLiteral(":/toolbar/result.png"),
            &MainWindow::showSimulationResults
        );
    simulationLockedActions << resultAction;

    toolBar->addSeparator();

    addBtn(QStringLiteral("系统设置"), QStringLiteral(":/toolbar/setup.png"), &MainWindow::settings, false);
    addBtn(QStringLiteral("关于"), QStringLiteral(":/toolbar/help.png"), &MainWindow::help, false);

    QAction *closeAppAct = new QAction(QIcon(QStringLiteral(":/toolbar/closeall.png")), QStringLiteral("关闭"), this);
    if (closeAppAct->icon().isNull()) {
        closeAppAct->setIcon(QIcon(QStringLiteral(":/toolbar/close.png")));
    }
    connect(closeAppAct, &QAction::triggered, this, &MainWindow::close);
    toolBar->addAction(closeAppAct);
}

void MainWindow::createTreeWidget()
{
    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderHidden(true);
    treeWidget->setColumnCount(1);

    treeWidget->header()->setStretchLastSection(false);
    treeWidget->header()->setSectionResizeMode(
        0,
        QHeaderView::ResizeToContents
    );

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
    rootItem->setText(0, QStringLiteral("浇注XX固化仿真工程"));
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
            adjustProjectPaneToContents();
            return;
        }
    }

    QTreeWidgetItem *projectItem = new QTreeWidgetItem(root);
    projectItem->setText(0, name);
    projectItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/file.png")));
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
    infoItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/information.png")));

    QFont childFont = infoItem->font(0);
    childFont.setPointSize(13);
    childFont.setBold(false);
    infoItem->setFont(0, childFont);

    QTreeWidgetItem *explosiveItem = new QTreeWidgetItem(projectItem);
    explosiveItem->setText(0, QStringLiteral("炸药参数"));
    explosiveItem->setData(0, Qt::UserRole, path);
    explosiveItem->setData(0, ROLE_NODE_TYPE, NODE_EXPLOSIVE);
    explosiveItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/cailiaocanshu.png")));
    explosiveItem->setFont(0, childFont);

    QTreeWidgetItem *structureItem = new QTreeWidgetItem(projectItem);
    structureItem->setText(0, QStringLiteral("结构参数"));
    structureItem->setData(0, Qt::UserRole, path);
    structureItem->setData(0, ROLE_NODE_TYPE, NODE_STRUCTURE);
    structureItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/jiegoucanshu.png")));
    structureItem->setFont(0, childFont);

    QTreeWidgetItem *moldItem = new QTreeWidgetItem(projectItem);
    moldItem->setText(0, QStringLiteral("模具参数"));
    moldItem->setData(0, Qt::UserRole, path);
    moldItem->setData(0, ROLE_NODE_TYPE, NODE_MOLD);
    moldItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/jiegoucanshu.png")));
    moldItem->setFont(0, childFont);

    QTreeWidgetItem *boundaryItem = new QTreeWidgetItem(projectItem);
    boundaryItem->setText(0, QStringLiteral("边界条件"));
    boundaryItem->setData(0, Qt::UserRole, path);
    boundaryItem->setData(0, ROLE_NODE_TYPE, NODE_BOUNDARY);
    boundaryItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/fangzhenshezhi.png")));
    boundaryItem->setFont(0, childFont);

    QTreeWidgetItem *simulationItem = new QTreeWidgetItem(projectItem);
    simulationItem->setText(0, QStringLiteral("仿真设置"));
    simulationItem->setData(0, Qt::UserRole, path);
    simulationItem->setData(0, ROLE_NODE_TYPE, NODE_SIMULATION);
    simulationItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/fangzhenshezhi.png")));
    simulationItem->setFont(0, childFont);

    QTreeWidgetItem *checkItem = new QTreeWidgetItem(projectItem);
    checkItem->setText(0, QStringLiteral("参数检查"));
    checkItem->setData(0, Qt::UserRole, path);
    checkItem->setData(0, ROLE_NODE_TYPE, NODE_PARAMETER_CHECK);
    checkItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/check.png")));
    checkItem->setFont(0, childFont);

    QTreeWidgetItem *startItem = new QTreeWidgetItem(projectItem);
    startItem->setText(0, QStringLiteral("开始仿真"));
    startItem->setData(0, Qt::UserRole, path);
    startItem->setData(0, ROLE_NODE_TYPE, NODE_START_SIMULATION);
    startItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/start.png")));
    startItem->setFont(0, childFont);

    QTreeWidgetItem *resultItem = new QTreeWidgetItem(projectItem);
    resultItem->setText(0, QStringLiteral("仿真结果"));
    resultItem->setData(0, Qt::UserRole, path);
    resultItem->setData(0, ROLE_NODE_TYPE, NODE_SIMULATION_RESULT);
    resultItem->setIcon(0, QIcon(QStringLiteral(":/toolbar/check.png")));
    resultItem->setFont(0, childFont);

    treeWidget->setCurrentItem(infoItem);
    root->setExpanded(true);
    adjustProjectPaneToContents();
}

void MainWindow::adjustProjectPaneToContents()
{
    if (!leftPaneWidget || !treeWidget) {
        return;
    }

    QTimer::singleShot(
        0,
        this,
        [this]() {
            if (!leftPaneWidget || !treeWidget) {
                return;
            }

            treeWidget->resizeColumnToContents(0);

            const int contentWidth =
                treeWidget->columnWidth(0);

            const int preferredWidth =
                qBound(
                    220,
                    contentWidth + 12,
                    600
                );

            leftPaneWidget->setFixedWidth(preferredWidth);

            QTimer::singleShot(
                0,
                this,
                [this]() {
                    updateHomeTitleFont();
                }
            );
        }
    );
}

void MainWindow::updateHomeTitleFont()
{
    if (!titleLabel) {
        return;
    }

    const int availableWidth =
        qMax(1, titleLabel->width() - 40);

    if (availableWidth <= 1) {
        return;
    }

    constexpr int maxFontPx = 136;
    constexpr int minFontPx = 62;

    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setWeight(QFont::Medium);
    font.setLetterSpacing(QFont::PercentageSpacing, 96.5);

    int targetFontPx = minFontPx;

    for (int fontPx = maxFontPx; fontPx >= minFontPx; --fontPx) {
        font.setPixelSize(fontPx);

        const QFontMetrics metrics(font);
        if (metrics.horizontalAdvance(titleLabel->text())
            <= availableWidth) {
            targetFontPx = fontPx;
            break;
        }
    }

    font.setPixelSize(targetFontPx);

    if (titleLabel->font().pixelSize() != targetFontPx) {
        titleLabel->setFont(font);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (homeTitleResizeTimer) {
        homeTitleResizeTimer->start();
    }
}

void MainWindow::restoreCurrentProjectTreeSelection()
{
    if (!treeWidget || !isProjectLoaded) {
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

        treeWidget->blockSignals(true);
        treeWidget->setCurrentItem(projectItem);
        treeWidget->scrollToItem(projectItem);
        treeWidget->blockSignals(false);
        return;
    }
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

void MainWindow::connectParameterEditSignals()
{
    connect(
        structureWidget,
        &BaseParamWidget::parameterEdited,
        this,
        [this]() { markParamPageDirty(ParamPage::Structure); }
    );
    connect(
        explosiveWidget,
        &BaseParamWidget::parameterEdited,
        this,
        [this]() { markParamPageDirty(ParamPage::Explosive); }
    );
    connect(
        moldWidget,
        &BaseParamWidget::parameterEdited,
        this,
        [this]() { markParamPageDirty(ParamPage::Mold); }
    );
    connect(
        boundaryWidget,
        &BaseParamWidget::parameterEdited,
        this,
        [this]() { markParamPageDirty(ParamPage::Boundary); }
    );
    connect(
        simulationWidget,
        &BaseParamWidget::parameterEdited,
        this,
        [this]() { markParamPageDirty(ParamPage::Simulation); }
    );
}

void MainWindow::markParamPageDirty(ParamPage page)
{
    dirtyParamPages.insert(page);
}

void MainWindow::clearParamPageDirty(ParamPage page)
{
    dirtyParamPages.remove(page);
}

void MainWindow::clearAllParamPageDirty()
{
    dirtyParamPages.clear();
}

bool MainWindow::ensureNoUnsavedParameters()
{
    if (dirtyParamPages.isEmpty()) {
        return true;
    }

    QStringList names;
    if (dirtyParamPages.contains(ParamPage::Structure)) {
        names << QStringLiteral("结构参数");
    }
    if (dirtyParamPages.contains(ParamPage::Explosive)) {
        names << QStringLiteral("炸药参数");
    }
    if (dirtyParamPages.contains(ParamPage::Mold)) {
        names << QStringLiteral("模具参数");
    }
    if (dirtyParamPages.contains(ParamPage::Boundary)) {
        names << QStringLiteral("边界条件");
    }
    if (dirtyParamPages.contains(ParamPage::Simulation)) {
        names << QStringLiteral("仿真设置");
    }

    QMessageBox msgBox(
        QMessageBox::Warning,
        QStringLiteral("存在未保存修改"),
        QStringLiteral(
            "当前存在未保存修改：%1\n\n"
            "请选择继续编辑，或放弃未保存修改。"
        ).arg(names.join(QStringLiteral("、"))),
        QMessageBox::NoButton,
        this
    );

    QPushButton *continueButton = msgBox.addButton(
        QStringLiteral("返回继续编辑"),
        QMessageBox::RejectRole
    );
    QPushButton *discardButton = msgBox.addButton(
        QStringLiteral("放弃未保存修改"),
        QMessageBox::DestructiveRole
    );
    msgBox.setDefaultButton(continueButton);
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.exec();

    if (msgBox.clickedButton() != discardButton) {
        return false;
    }

    QString reloadError;
    if (!reloadParameterPagesFromSavedConfig(reloadError)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("无法放弃修改"),
            reloadError
        );
        return false;
    }

    clearAllParamPageDirty();
    return true;
}

bool MainWindow::ensureValidProjectJobName()
{
    if (!isProjectLoaded) {
        return false;
    }

    QString nameError;
    if (ProjectManager::isValidProjectName(
            currentProject.projectName,
            nameError)) {
        return true;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Warning,
        QStringLiteral("工程目录名称无效"),
        QStringLiteral(
            "工程目录名称不符合 Abaqus Job 命名要求，"
            "请重命名工程目录。\n\n"
            "当前目录名：%1"
        ).arg(currentProject.projectName)
    );

    return false;
}

bool MainWindow::promptAndClearStaleJobLock()
{
    if (!simulationManager->hasLockFiles()) {
        return true;
    }

    const QString lockPath = simulationManager->currentJobLockPath();
    const QString jobName = QFileInfo(lockPath).completeBaseName();

    QMessageBox msgBox(
        QMessageBox::Warning,
        QStringLiteral("检测到 Job 锁文件"),
        QStringLiteral(
            "检测到当前工程 Job 锁文件：\n%1\n\n"
            "如果确认 Abaqus 已经没有运行，可以清理残留锁。"
        ).arg(jobName),
        QMessageBox::NoButton,
        this
    );

    QPushButton *clearButton = msgBox.addButton(
        QStringLiteral("清理残留锁"),
        QMessageBox::AcceptRole
    );
    QPushButton *cancelButton = msgBox.addButton(
        QStringLiteral("取消"),
        QMessageBox::RejectRole
    );
    msgBox.setDefaultButton(cancelButton);
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.exec();

    if (msgBox.clickedButton() != clearButton) {
        return false;
    }

    QString error;
    if (!simulationManager->clearCurrentJobLock(error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Critical,
            QStringLiteral("清理失败"),
            error
        );
        return false;
    }

    return !simulationManager->hasLockFiles();
}

void MainWindow::loadProjectToUi()
{
    clearAllParamPageDirty();
    projectDirectoryMissing = false;
    if (resultViewerWidget) {
        resultViewerWidget->stopPlayback();
        resultViewerWidget->setProjectPath(QString());
    }
    infoWidget->setProjectData(currentProject);
    {
        QString reloadError;
        if (!reloadParameterPagesFromSavedConfig(reloadError)) {
            structureWidget->setConfig(StructureConfig());
            explosiveWidget->setConfig(ExplosiveConfig());
            moldWidget->setConfig(MoldConfig());
            boundaryWidget->setConfig(BoundaryConfig());
            simulationWidget->setConfig(SimulationConfig());

            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("工程参数读取异常"),
                reloadError
                    + QStringLiteral(
                        "\n\n部分参数文件可能损坏，"
                        "请进入参数检查确认。"
                    )
            );
        }
    }
    setWindowTitle(QStringLiteral("浇注XX固化监测与三维参数重构分析软件 - %1").arg(currentProject.projectName));
    selectTreeItem(QStringLiteral("工程信息"));
    stackedWidget->setCurrentWidget(infoWidget);
    updateUIStates();
}

void MainWindow::newProject()
{
    if (!ensureNoUnsavedParameters()) {
        return;
    }

    if (!ensureSimulationIdle(QStringLiteral("新建工程"))) {
        return;
    }

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
    if (!ensureNoUnsavedParameters()) {
        return;
    }

    if (!ensureSimulationIdle(QStringLiteral("打开其他工程"))) {
        return;
    }

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

void MainWindow::exitProject()
{
    if (!isProjectLoaded) {
        return;
    }

    if (simulationManager->isActive()) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("提示"),
            QStringLiteral(
                "当前仿真正在进行或正在终止，"
                "请先点击“终止仿真”，"
                "并等待终止完成后再关闭工程。"
            )
        );
        return;
    }

    if (!ensureNoUnsavedParameters()) {
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
        setWindowTitle(QStringLiteral("浇注XX固化监测与三维参数重构分析软件"));
        treeWidget->clearSelection();
    } else if (root && root->childCount() > 0) {
        QTreeWidgetItem *nextProject = root->child(0);

        treeWidget->setCurrentItem(nextProject);
        onTreeItemClicked(nextProject, 0);

        // 工程树发生变化后重新构建目录监控
        startWatchingProject();
    }

    adjustProjectPaneToContents();
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

    if (dirtyParamPages.contains(ParamPage::Structure)) {
        selectTreeItem(QStringLiteral("结构参数"));
        stackedWidget->setCurrentWidget(structureWidget);
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

    if (!ensureParameterWritable(QStringLiteral("结构参数"))) {
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
    clearParamPageDirty(ParamPage::Structure);
}

void MainWindow::explosiveParams()
{
    if (!isProjectLoaded) {
        return;
    }

    if (dirtyParamPages.contains(ParamPage::Explosive)) {
        selectTreeItem(QStringLiteral("炸药参数"));
        stackedWidget->setCurrentWidget(explosiveWidget);
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

    if (!ensureParameterWritable(QStringLiteral("炸药参数"))) {
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
    clearParamPageDirty(ParamPage::Explosive);
}

void MainWindow::moldParams()
{
    if (!isProjectLoaded) {
        return;
    }

    if (dirtyParamPages.contains(ParamPage::Mold)) {
        selectTreeItem(QStringLiteral("模具参数"));
        stackedWidget->setCurrentWidget(moldWidget);
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

    if (!ensureParameterWritable(QStringLiteral("模具参数"))) {
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
    clearParamPageDirty(ParamPage::Mold);
}

void MainWindow::boundaryParams()
{
    if (!isProjectLoaded) {
        return;
    }

    if (dirtyParamPages.contains(ParamPage::Boundary)) {
        selectTreeItem(QStringLiteral("边界条件"));
        stackedWidget->setCurrentWidget(boundaryWidget);
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

    if (!ensureParameterWritable(QStringLiteral("边界条件"))) {
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
    clearParamPageDirty(ParamPage::Boundary);
}

void MainWindow::simulationParams()
{
    if (!isProjectLoaded) {
        return;
    }

    if (dirtyParamPages.contains(ParamPage::Simulation)) {
        selectTreeItem(QStringLiteral("仿真设置"));
        stackedWidget->setCurrentWidget(simulationWidget);
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

    if (!ensureParameterWritable(QStringLiteral("仿真设置"))) {
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
    clearParamPageDirty(ParamPage::Simulation);
}

void MainWindow::checkParams()
{
    if (!isProjectLoaded) {
        return;
    }

    if (!ensureNoUnsavedParameters()) {
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

    if (!ensureNoUnsavedParameters()) {
        return;
    }

    if (!ensureSimulationIdle(QStringLiteral("重新生成 Abaqus 文件"))) {
        return;
    }

    if (!ensureValidProjectJobName()) {
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


void MainWindow::connectSimulationManager()
{
    connect(
        simulationManager,
        &SimulationManager::stateChanged,
        this,
        &MainWindow::onSimulationStateChanged
    );
    connect(
        simulationManager,
        &SimulationManager::statusChanged,
        simulationMonitorWidget,
        &SimulationMonitorWidget::setStatus
    );
    connect(
        simulationManager,
        &SimulationManager::progressUpdated,
        simulationMonitorWidget,
        &SimulationMonitorWidget::setProgress
    );
    connect(
        simulationManager,
        &SimulationManager::logReceived,
        simulationMonitorWidget,
        &SimulationMonitorWidget::appendLog
    );
    connect(
        simulationManager,
        &SimulationManager::monitorResetRequested,
        this,
        [this]() {
            simulationMonitorWidget->clearLog();
            simulationMonitorWidget->setProgress(0);
        }
    );
    connect(
        simulationManager,
        &SimulationManager::simulationFinished,
        this,
        &MainWindow::onSimulationFinished
    );
    connect(
        simulationManager,
        &SimulationManager::errorOccurred,
        this,
        &MainWindow::onSimulationErrorOccurred
    );
    connect(
        simulationManager,
        &SimulationManager::forceKillRequested,
        this,
        &MainWindow::onForceKillRequested
    );
}

void MainWindow::onSimulationStateChanged(SimulationState state)
{
    setParameterPagesReadOnly(simulationManager->isActive());
    updateUIStates();

    if (state == SimulationState::Stopped
        || state == SimulationState::Finished
        || state == SimulationState::Failed
        || state == SimulationState::PostProcessFailed) {
        if (projectDirectoryMissing
            && isProjectLoaded
            && QFileInfo(currentProject.projectPath).exists()) {
            projectDirectoryMissing = false;
        }

        if (projectDirectoryMissing) {
            const QString missingProjectPath =
                currentProject.projectPath;

            stopWatchingProject();

            removeProjectTreeItemByPath(
                missingProjectPath
            );

            isProjectLoaded = false;
            currentProject = ProjectConfig();
            projectDirectoryMissing = false;

            stackedWidget->setCurrentIndex(0);

            setWindowTitle(
                QStringLiteral(
                    "浇注XX固化监测与三维参数重构分析软件"
                )
            );

            treeWidget->clearSelection();

            updateUIStates();
            startWatchingProject();
        }
    }

    if (state == SimulationState::T0Running
        || state == SimulationState::T1Running
        || state == SimulationState::T2Running
        || state == SimulationState::Stopping) {
        selectTreeItem(QStringLiteral("开始仿真"));
        stackedWidget->setCurrentWidget(simulationMonitorWidget);
    }
}

void MainWindow::onForceKillRequested()
{
    QMessageBox msgBox(
        QMessageBox::Warning,
        QStringLiteral("终止等待时间过长"),
        QStringLiteral(
            "Abaqus Job 已等待约120秒，"
            "锁文件仍未释放。\n\n"
            "选择“继续等待”将延长等待；"
            "只有明确选择“强制结束”才会"
            "终止本软件启动的 Abaqus 进程。"
        ),
        QMessageBox::NoButton,
        this
    );

    QPushButton *waitButton =
        msgBox.addButton(
            QStringLiteral("继续等待"),
            QMessageBox::AcceptRole
        );
    QPushButton *forceButton =
        msgBox.addButton(
            QStringLiteral("强制结束"),
            QMessageBox::DestructiveRole
        );

    msgBox.setDefaultButton(waitButton);
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.exec();

    const bool userChoseForceKill =
        msgBox.clickedButton() == forceButton;

    simulationManager->respondToForceKillPrompt(!userChoseForceKill);
}

void MainWindow::onSimulationErrorOccurred(
    const QString &title,
    const QString &text)
{
    QMessageBox::Icon icon = QMessageBox::Warning;
    if (title == QStringLiteral("错误")) {
        icon = QMessageBox::Critical;
    } else if (title == QStringLiteral("完成")) {
        icon = QMessageBox::Information;
    }

    showCenteredMessageBox(this, icon, title, text);
}

void MainWindow::onSimulationFinished()
{
    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("完成"),
        QStringLiteral("固化仿真完成。")
    );
}

bool MainWindow::ensureSimulationIdle(const QString &operation)
{
    if (!simulationManager->isActive()) {
        return true;
    }

    stackedWidget->setCurrentWidget(simulationMonitorWidget);
    selectTreeItem(QStringLiteral("开始仿真"));

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("仿真正在进行"),
        QStringLiteral(
            "当前已有 Abaqus 仿真正在运行，"
            "暂不能%1。"
        ).arg(operation)
    );

    return false;
}

bool MainWindow::ensureParameterWritable(const QString &parameterName)
{
    if (!simulationManager->isActive()) {
        return true;
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("仿真正在进行"),
        QStringLiteral(
            "当前 Abaqus 仿真正在运行，"
            "%1仅供查看，暂时不能修改或保存。"
        ).arg(parameterName)
    );

    return false;
}

void MainWindow::showSimulationPreparePage()
{
    if (!isProjectLoaded
        || !simulationPrepareWidget
        || !simulationMonitorWidget) {
        return;
    }

    if (!ensureNoUnsavedParameters()) {
        return;
    }

    if (!ensureValidProjectJobName()) {
        return;
    }

    selectTreeItem(QStringLiteral("开始仿真"));

    if (simulationManager->isActive()) {
        stackedWidget->setCurrentWidget(simulationMonitorWidget);
        return;
    }

    simulationManager->setProjectContext(
        currentProject.projectPath,
        SettingsDialog::getAbaqusPath()
    );
    simulationManager->setForceFullRerun(false);

    QString previousResultMessage;
    if (simulationManager->hasValidPreviousResult(previousResultMessage)) {
        const PreviousResultAction action =
            promptPreviousSimulationResult(this, previousResultMessage);

        if (action == PreviousResultViewLogs) {
            showPreviousSimulationLogs();
            return;
        }

        if (action == PreviousResultCancel) {
            return;
        }

        if (action == PreviousResultRerun) {
            simulationManager->setForceFullRerun(true);
        }
    }

    QString error;
    const bool ready = simulationManager->checkReady(error);

    simulationPrepareWidget->setReadyState(ready, error);

    stackedWidget->setCurrentWidget(simulationPrepareWidget);
}

void MainWindow::startSimulation()
{
    if (!isProjectLoaded) {
        return;
    }

    if (!ensureNoUnsavedParameters()) {
        return;
    }

    if (!ensureValidProjectJobName()) {
        return;
    }

    if (simulationManager->isActive()) {
        selectTreeItem(QStringLiteral("开始仿真"));
        stackedWidget->setCurrentWidget(simulationMonitorWidget);
        return;
    }

    if (!promptAndClearStaleJobLock()) {
        return;
    }

    QString error;
    if (!simulationManager->checkReady(error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("仿真无法启动"),
            error
        );
        return;
    }

    {
        QString reloadError;
        if (!reloadParameterPagesFromSavedConfig(reloadError)) {
            showCenteredMessageBox(
                this,
                QMessageBox::Warning,
                QStringLiteral("仿真无法启动"),
                reloadError
            );
            return;
        }
    }

    const QString abaqusPath = SettingsDialog::getAbaqusPath();
    simulationManager->setProjectContext(
        currentProject.projectPath,
        abaqusPath
    );

    const SimulationResumeMode resumeMode =
        simulationManager->detectResumeMode();
    if (resumeMode == SimulationResumeMode::PostProcessOnly) {
        const PostProcessResumeAction action =
            promptPostProcessResume(
                this,
                simulationManager->resumeModeMessage(
                    resumeMode
                )
            );

        if (action == PostProcessResumeCancel) {
            return;
        }

        if (action == PostProcessResumeFullRun) {
            simulationManager->setForceFullRerun(true);
        }
    }

    selectTreeItem(QStringLiteral("开始仿真"));
    stackedWidget->setCurrentWidget(simulationMonitorWidget);

    simulationManager->startTask(
        currentProject.projectPath,
        abaqusPath
    );
}

void MainWindow::stopSimulation()
{
    const SimulationState state = simulationManager->state();
    if (state != SimulationState::T0Running
        && state != SimulationState::T1Running
        && state != SimulationState::T2Running) {
        showCenteredMessageBox(
            this,
            QMessageBox::Information,
            QStringLiteral("提示"),
            QStringLiteral("当前没有正在运行的仿真。")
        );
        return;
    }

    QString confirmText =
        QStringLiteral(
            "确定终止当前Abaqus仿真吗？\n"
            "未完成结果可能无法保存。"
        );
    if (state == SimulationState::T2Running) {
        confirmText =
            QStringLiteral(
                "确定终止当前后处理吗？\n"
                "Abaqus 求解结果和已生成的图片/视频将保留。"
            );
    }

    if (showCenteredMessageBox(
            this,
            QMessageBox::Question,
            QStringLiteral("终止仿真"),
            confirmText,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        ) != QMessageBox::Yes) {
        return;
    }

    simulationManager->stopTask();
}

void MainWindow::updateUIStates()
{
    for (QAction *act : projectDependentActions) {
        act->setEnabled(isProjectLoaded);
    }

    const bool running = simulationManager->isActive();

    for (QAction *act : simulationLockedActions) {
        if (act) {
            act->setEnabled(isProjectLoaded && !running);
        }
    }

    if (stopSimulationAction) {
        const SimulationState state = simulationManager->state();
        const bool canStop =
            state == SimulationState::T0Running
            || state == SimulationState::T1Running
            || state == SimulationState::T2Running;

        stopSimulationAction->setEnabled(canStop);
    }
}

void MainWindow::setParameterPagesReadOnly(bool readOnly)
{
    if (structureWidget) {
        structureWidget->setReadOnlyMode(readOnly);
    }

    if (explosiveWidget) {
        explosiveWidget->setReadOnlyMode(readOnly);
    }

    if (moldWidget) {
        moldWidget->setReadOnlyMode(readOnly);
    }

    if (boundaryWidget) {
        boundaryWidget->setReadOnlyMode(readOnly);
    }

    if (simulationWidget) {
        simulationWidget->setReadOnlyMode(readOnly);
    }
}

bool MainWindow::reloadParameterPagesFromSavedConfig(QString &errorMessage)
{
    if (!isProjectLoaded) {
        errorMessage = QStringLiteral("当前未打开工程。");
        return false;
    }

    const QDir projectDir(currentProject.projectPath);

    StructureConfig structure;
    ExplosiveConfig explosive;
    MoldConfig mold;
    BoundaryConfig boundary;
    SimulationConfig simulation;

    const QString structurePath =
        projectDir.filePath(QStringLiteral("config/structure.json"));
    const QString explosivePath =
        projectDir.filePath(QStringLiteral("config/explosive.json"));
    const QString moldPath =
        projectDir.filePath(QStringLiteral("config/mold.json"));
    const QString boundaryPath =
        projectDir.filePath(QStringLiteral("config/boundary.json"));
    const QString simulationPath =
        projectDir.filePath(QStringLiteral("config/simulation.json"));

    if (QFileInfo::exists(structurePath)
        && !StructureConfigManager::load(
            currentProject.projectPath,
            structure)) {
        errorMessage =
            QStringLiteral("结构参数配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(explosivePath)
        && !ExplosiveConfigManager::load(
            currentProject.projectPath,
            explosive)) {
        errorMessage =
            QStringLiteral("炸药参数配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(moldPath)
        && !MoldConfigManager::load(
            currentProject.projectPath,
            mold)) {
        errorMessage =
            QStringLiteral("模具参数配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(boundaryPath)
        && !BoundaryConfigManager::load(
            currentProject.projectPath,
            boundary)) {
        errorMessage =
            QStringLiteral("边界条件配置文件无效或无法读取。");
        return false;
    }

    if (QFileInfo::exists(simulationPath)
        && !SimulationConfigManager::load(
            currentProject.projectPath,
            simulation)) {
        errorMessage =
            QStringLiteral("仿真设置配置文件无效或无法读取。");
        return false;
    }

    structureWidget->setConfig(structure);
    explosiveWidget->setConfig(explosive);
    moldWidget->setConfig(mold);
    boundaryWidget->setConfig(boundary);
    simulationWidget->setConfig(simulation);

    errorMessage.clear();
    return true;
}

void MainWindow::removeProjectTreeItemByPath(const QString &path)
{
    if (!treeWidget || path.isEmpty()) {
        return;
    }

    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    if (!root) {
        return;
    }

    for (int i = 0; i < root->childCount(); ++i) {
        QTreeWidgetItem *projectItem = root->child(i);
        if (projectItem->data(0, Qt::UserRole).toString() == path) {
            delete projectItem;
            return;
        }
    }
}

void MainWindow::showSimulationResults()
{
    if (!isProjectLoaded || !resultViewerWidget) {
        return;
    }

    if (!ensureNoUnsavedParameters()) {
        return;
    }

    if (simulationManager->isActive()) {
        showCenteredMessageBox(
            this,
            QMessageBox::Information,
            QStringLiteral("仿真正在进行"),
            QStringLiteral(
                "当前工程正在进行 Abaqus 仿真或后处理，"
                "请等待本次计算完成后查看结果。"
            )
        );
        selectTreeItem(QStringLiteral("开始仿真"));
        stackedWidget->setCurrentWidget(simulationMonitorWidget);
        return;
    }

    resultViewerWidget->stopPlayback();
    resultViewerWidget->setProjectPath(currentProject.projectPath);

    selectTreeItem(QStringLiteral("仿真结果"));
    stackedWidget->setCurrentWidget(resultViewerWidget);

    QTimer::singleShot(
        0,
        resultViewerWidget,
        [this]() {
            if (stackedWidget->currentWidget()
                == resultViewerWidget) {
                resultViewerWidget->refreshResults();
            }
        }
    );
}

void MainWindow::openResultsDirectory()
{
    if (!isProjectLoaded) {
        return;
    }

    const QString resultsDir =
        SimulationResultService::resultsDirectoryPath(
            currentProject.projectPath
        );

    if (!QFileInfo(resultsDir).exists()) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("结果目录不存在"),
            QStringLiteral("当前工程尚未生成 results 目录。")
        );
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(resultsDir));
}

void MainWindow::generateSimulationReport()
{
    if (!isProjectLoaded) {
        return;
    }

    if (!dirtyParamPages.isEmpty()) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("参数未保存"),
            QStringLiteral(
                "当前存在未保存的参数修改，"
                "请先保存参数后再生成报告。"
            )
        );
        return;
    }

    if (simulationManager->isActive()) {
        showCenteredMessageBox(
            this,
            QMessageBox::Information,
            QStringLiteral("仿真正在进行"),
            QStringLiteral(
                "当前工程正在进行 Abaqus 仿真或后处理，"
                "请等待本次计算完成后再生成报告。"
            )
        );
        return;
    }

    const ResultValidationResult validation =
        SimulationResultService::validate(currentProject.projectPath);

    if (!validation.isValid()) {
        QString title = QStringLiteral("无法生成报告");
        if (validation.state == ResultValidationState::PostIncomplete) {
            title = QStringLiteral("后处理未完成");
        } else if (validation.state == ResultValidationState::PostShaMismatch) {
            title = QStringLiteral("结果与参数不一致");
        }

        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            title,
            validation.message
        );
        return;
    }

    QString error;
    if (!SimulationReportGenerator::generate(
            currentProject.projectPath,
            error)) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("报告生成失败"),
            error
        );
        return;
    }

    const QString pdfPath =
        SimulationResultService::reportPdfPath(
            currentProject.projectPath
        );

    if (resultViewerWidget) {
        resultViewerWidget->updateReportButtonText();
    }

    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("报告已生成"),
        QStringLiteral("PDF 报告已保存至：\n%1").arg(pdfPath)
    );
}

void MainWindow::showPreviousSimulationLogs()
{
    if (!isProjectLoaded || !simulationMonitorWidget) {
        return;
    }

    const QString projectDir = currentProject.projectPath;

    const QString abaqusDir =
        QDir(projectDir).filePath(QStringLiteral("abaqus"));

    const QString logsDir =
        QDir(projectDir).filePath(QStringLiteral("logs"));

    const QString jobName =
        ProjectInputHash::currentJobName(projectDir);

    QStringList logSections;

    auto loadLog =
        [&logSections](const QString &title, const QString &path) {
            QFile file(path);

            if (!file.exists()) {
                return;
            }

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return;
            }

            const QString content =
                QString::fromLocal8Bit(file.readAll());

            file.close();

            logSections
                << QStringLiteral("========== %1 ==========").arg(title)
                << content;
        };

    loadLog(
        QStringLiteral("T0 LOG"),
        QDir(logsDir).filePath(QStringLiteral("t0.log"))
    );
    loadLog(
        QStringLiteral("T1 LOG"),
        QDir(logsDir).filePath(QStringLiteral("t1.log"))
    );
    loadLog(
        QStringLiteral("ABAQUS MSG"),
        QDir(abaqusDir).filePath(jobName + QStringLiteral(".msg"))
    );
    loadLog(
        QStringLiteral("ABAQUS STA"),
        QDir(abaqusDir).filePath(jobName + QStringLiteral(".sta"))
    );
    loadLog(
        QStringLiteral("T2 LOG"),
        QDir(logsDir).filePath(QStringLiteral("t2.log"))
    );

    simulationMonitorWidget->clearLog();
    simulationMonitorWidget->setLogText(
        logSections.isEmpty()
            ? QStringLiteral("未找到上一次仿真日志文件。")
            : logSections.join(QStringLiteral("\n\n"))
    );

    simulationMonitorWidget->setStatus(
        QStringLiteral("上次仿真已正常完成")
    );
    simulationMonitorWidget->setProgress(100);

    selectTreeItem(QStringLiteral("开始仿真"));
    stackedWidget->setCurrentWidget(simulationMonitorWidget);
}

void MainWindow::settings()
{
    if (!ensureSimulationIdle(QStringLiteral("修改系统设置"))) {
        return;
    }

    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::help()
{
    showCenteredMessageBox(
        this,
        QMessageBox::Information,
        QStringLiteral("关于"),
        QStringLiteral(
            "浇注XX固化监测与三维参数重构分析软件\n\n"
            "版本 1.0"
        )
    );
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
    if (simulationManager->isActive()) {
        showCenteredMessageBox(
            this,
            QMessageBox::Warning,
            QStringLiteral("提示"),
            QStringLiteral(
                "当前仿真正在进行或正在终止，"
                "请先终止仿真并等待终止完成后再退出软件。"
            )
        );
        event->ignore();
        return;
    }

    if (!ensureNoUnsavedParameters()) {
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
             || nodeType == NODE_START_SIMULATION
             || nodeType == NODE_SIMULATION_RESULT) {
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

    if (simulationManager->isActive()
        && !simulationManager->projectPath().isEmpty()
        && path != simulationManager->projectPath()) {

        showCenteredMessageBox(
            this,
            QMessageBox::Information,
            QStringLiteral("仿真正在进行"),
            QStringLiteral(
                "当前工程正在执行 Abaqus 仿真，"
                "仿真结束或终止前不能切换到其他工程。"
            )
        );

        selectTreeItem(QStringLiteral("开始仿真"));
        stackedWidget->setCurrentWidget(simulationMonitorWidget);

        return;
    }

    // 如果切换到了另一个工程，重新加载该工程
    if (!isProjectLoaded ||
        path != currentProject.projectPath) {

        if (!ensureNoUnsavedParameters()) {
            restoreCurrentProjectTreeSelection();
            return;
        }

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
            adjustProjectPaneToContents();
            startWatchingProject();
            return;
        }

        currentProject = config;
        isProjectLoaded = true;

        if (resultViewerWidget) {
            resultViewerWidget->stopPlayback();
            resultViewerWidget->setProjectPath(QString());
        }

        updateUIStates();
    }

    // 更新窗口标题
    setWindowTitle(
        QStringLiteral(
            "浇注XX固化监测与三维参数重构分析软件 - %1"
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
        showSimulationPreparePage();
        return;
    }

    if (nodeType == NODE_SIMULATION_RESULT) {
        showSimulationResults();
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
        if (simulationManager->isActive()
            && isProjectLoaded
            && item->data(0, Qt::UserRole).toString() == currentProject.projectPath) {
            projectDirectoryMissing = true;
            if (simulationMonitorWidget) {
                simulationMonitorWidget->appendLog(
                    QStringLiteral("[SYS] 工程目录暂不可访问")
                );
            }
            continue;
        }

        delete item;
    }

    adjustProjectPaneToContents();

    if (currentProjectDeleted && !simulationManager->isActive()) {
        stopWatchingProject();
        isProjectLoaded = false;
        currentProject = ProjectConfig();
        stackedWidget->setCurrentIndex(0);
        setWindowTitle(QStringLiteral("浇注XX固化监测与三维参数重构分析软件"));
        updateUIStates();
        startWatchingProject();
    }
}

