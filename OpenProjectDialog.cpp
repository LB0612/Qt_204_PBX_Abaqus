#include "OpenProjectDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

OpenProjectDialog::OpenProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("打开工程"));
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    resize(700, 500);

    setupUi();
    applyStyles();
    scanProjects();
}

void OpenProjectDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QHBoxLayout *topLayout = new QHBoxLayout();

    QLabel *pathLabel = new QLabel(QStringLiteral("工程位置:"));
    rootPathEdit = new QLineEdit();

    QSettings settings(
        QStringLiteral("PBXSimulationSoftware"),
        QStringLiteral("OpenProjectDialog")
    );
    QString lastOpenProjectPath = settings.value(
        QStringLiteral("lastOpenProjectPath"),
        QDir::homePath()
    ).toString();
    if (!QDir(lastOpenProjectPath).exists()) {
        lastOpenProjectPath = QDir::homePath();
    }
    rootPathEdit->setText(lastOpenProjectPath);
    rootPathEdit->setReadOnly(true);

    browseBtn = new QPushButton(QStringLiteral("浏览工作区"));
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setFixedHeight(36);
    browseBtn->setMinimumWidth(100);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择工程所在的父级文件夹"), rootPathEdit->text());
        if (!dir.isEmpty()) {
            rootPathEdit->setText(dir);

            QSettings settings(
                QStringLiteral("PBXSimulationSoftware"),
                QStringLiteral("OpenProjectDialog")
            );
            settings.setValue(
                QStringLiteral("lastOpenProjectPath"),
                dir
            );

            scanProjects();
        }
    });

    topLayout->addWidget(pathLabel);
    topLayout->addWidget(rootPathEdit, 1);
    topLayout->addWidget(browseBtn);
    mainLayout->addLayout(topLayout);

    projectTable = new QTableWidget();
    projectTable->setColumnCount(4);
    projectTable->setHorizontalHeaderLabels({
        QStringLiteral("工程名称"),
        QStringLiteral("工程类型"),
        QStringLiteral("创建时间"),
        QStringLiteral("路径")
    });
    projectTable->horizontalHeader()->setMinimumHeight(45);
    projectTable->verticalHeader()->setDefaultSectionSize(40);
    projectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    projectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    projectTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    projectTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectTable->verticalHeader()->setVisible(false);
    mainLayout->addWidget(projectTable);

    connect(projectTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        projectTable->selectRow(row);
        openBtn->click();
    });

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setObjectName(QStringLiteral("CancelBtn"));
    cancelBtn->setFixedSize(100, 36);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    openBtn = new QPushButton(QStringLiteral("打开选中工程"));
    openBtn->setObjectName(QStringLiteral("OpenBtn"));
    openBtn->setFixedSize(140, 36);
    openBtn->setEnabled(false);
    connect(openBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(openBtn);
    mainLayout->addLayout(btnLayout);

    connect(projectTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        openBtn->setEnabled(!projectTable->selectedItems().isEmpty());
    });
}

void OpenProjectDialog::scanProjects()
{
    const QString rootPath = rootPathEdit->text();
    QDir dir(rootPath);
    if (!dir.exists()) {
        return;
    }

    projectTable->setRowCount(0);
    openBtn->setEnabled(false);

    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDirName : subDirs) {
        const QString fullPath = dir.filePath(subDirName);
        ProjectConfig config;
        if (!ProjectManager::loadProject(fullPath, config)) {
            continue;
        }

        const int row = projectTable->rowCount();
        projectTable->insertRow(row);
        projectTable->setItem(row, 0, new QTableWidgetItem(config.projectName));
        projectTable->setItem(row, 1, new QTableWidgetItem(QStringLiteral("浇注XX固化监测与三维参数重构分析软件")));
        projectTable->setItem(row, 2, new QTableWidgetItem(config.createdDate));
        projectTable->setItem(row, 3, new QTableWidgetItem(config.projectPath));
    }
}

QStringList OpenProjectDialog::getSelectedPaths() const
{
    QStringList paths;
    const QModelIndexList selectedRows = projectTable->selectionModel()->selectedRows();
    for (const QModelIndex &index : selectedRows) {
        const QString path = projectTable->item(index.row(), 3)->text();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    return paths;
}

void OpenProjectDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
            font-family: 'Microsoft YaHei';
        }
        QLabel { font-size: 18px; color: #333333; font-weight: bold; }
        QLineEdit {
            border: 1px solid #CCCCCC; border-radius: 4px; padding: 5px; background: #FAFAFA; font-size: 16px;
        }
        QTableWidget {
            border: 1px solid #dcdcdc;
            gridline-color: #eeeeee;
            selection-background-color: #e6f7ff;
            selection-color: #000000;
            font-size: 16px;
        }
        QHeaderView::section {
            background-color: #f5f7fa;
            border: none;
            border-bottom: 1px solid #dcdcdc;
            padding: 8px;
            font-weight: bold;
            font-size: 18px;
        }
        QPushButton {
            border: 1px solid #CCCCCC; border-radius: 4px; background-color: #F0F0F0; padding: 6px 15px; font-size: 16px;
        }
        QPushButton:hover { background-color: #E0E0E0; }
        QPushButton#OpenBtn { background-color: #0078D7; color: white; border: none; }
        QPushButton#OpenBtn:hover { background-color: #1084E3; }
        QPushButton#OpenBtn:disabled { background-color: #CCCCCC; color: #EEEEEE; }
    )");
}
