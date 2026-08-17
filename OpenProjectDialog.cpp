#include "OpenProjectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QDir>
#include <QLabel>
#include <utility>

OpenProjectDialog::OpenProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("打开工程");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    resize(700, 500); // 稍微大一点，以便显示列表

    setupUi();
    applyStyles();

    // 默认扫描当前用户目录或上一次的目录
    scanProjects();
}

void OpenProjectDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // --- 顶部筛选区 ---
    QHBoxLayout *topLayout = new QHBoxLayout();

    QLabel *pathLabel = new QLabel("工程位置:");
    rootPathEdit = new QLineEdit();
    rootPathEdit->setText(QDir::homePath()); // 默认路径，建议改为保存上次路径
    rootPathEdit->setReadOnly(true);

    browseBtn = new QPushButton("浏览工作区");
    browseBtn->setCursor(Qt::PointingHandCursor);
    connect(browseBtn, &QPushButton::clicked, [this](){
        QString dir = QFileDialog::getExistingDirectory(this, "选择工程所在的父级文件夹", rootPathEdit->text());
        if (!dir.isEmpty()) {
            rootPathEdit->setText(dir);
            scanProjects(); // 路径变了，重新扫描
        }
    });

    QLabel *filterLabel = new QLabel("工艺筛选:");
    typeFilterCombo = new QComboBox();
    typeFilterCombo->addItems({"全部显示", "捏合混匀工艺", "挤压造粒工艺", "压制成型工艺"});
    typeFilterCombo->setCursor(Qt::PointingHandCursor);
    // 当筛选改变时，重新刷新列表
    connect(typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OpenProjectDialog::scanProjects);

    topLayout->addWidget(pathLabel);
    topLayout->addWidget(rootPathEdit, 1);
    topLayout->addWidget(browseBtn);
    topLayout->addSpacing(20);
    topLayout->addWidget(filterLabel);
    topLayout->addWidget(typeFilterCombo);

    mainLayout->addLayout(topLayout);

    // --- 中间列表区 ---
    projectTable = new QTableWidget();
    projectTable->setColumnCount(4);
    projectTable->setHorizontalHeaderLabels({"工程名称", "工艺类型", "创建时间", "路径"});
    projectTable->horizontalHeader()->setMinimumHeight(45);
    projectTable->verticalHeader()->setDefaultSectionSize(40);
    projectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    projectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    projectTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    projectTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectTable->verticalHeader()->setVisible(false);

    mainLayout->addWidget(projectTable);

    // 双击列表直接打开
    connect(projectTable, &QTableWidget::cellDoubleClicked, [this](int row, int col){
        Q_UNUSED(col);
        projectTable->selectRow(row);
        openBtn->click();
    });

    // --- 底部按钮区 ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("CancelBtn");
    cancelBtn->setFixedSize(100, 35);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    openBtn = new QPushButton("打开选中工程");
    openBtn->setObjectName("OpenBtn");
    openBtn->setFixedSize(140, 35);
    openBtn->setEnabled(false); // 初始不可点
    connect(openBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(openBtn);
    mainLayout->addLayout(btnLayout);

    // 选中行时启用打开按钮
    connect(projectTable, &QTableWidget::itemSelectionChanged, [this](){
        openBtn->setEnabled(!projectTable->selectedItems().isEmpty());
    });
}

void OpenProjectDialog::scanProjects()
{
    QString rootPath = rootPathEdit->text();
    QDir dir(rootPath);
    if (!dir.exists()) return;

    projectTable->setRowCount(0);
    allProjects.clear();
    openBtn->setEnabled(false);

    // 获取所有子文件夹
    QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    int filterType = typeFilterCombo->currentIndex(); // 0:全部, 1:捏合, 2:挤压...

    for (const QString &subDirName : std::as_const(subDirs)) {
        QString fullPath = dir.filePath(subDirName);

        // 尝试加载 project.json
        ProjectConfig config;
        if (ProjectManager::loadProject(fullPath, config)) {
            // --- 核心筛选逻辑 ---
            // 如果 filterType 为 0 (全部)，则通过
            // 否则，比较 config.processType (0,1,2) 与 filterType - 1
            if (filterType != 0 && config.processType != (filterType - 1)) {
                continue; // 跳过不符合类型的工程
            }

            // 添加到表格
            int row = projectTable->rowCount();
            projectTable->insertRow(row);

            projectTable->setItem(row, 0, new QTableWidgetItem(config.projectName));

            QString typeStr;
            if(config.processType == 0) {
                typeStr = "捏合混匀";
            }
            else if(config.processType == 1) {
                typeStr = "挤压造粒";
            }
            else {
                typeStr = "压制成型";
            }
            projectTable->setItem(row, 1, new QTableWidgetItem(typeStr));

            projectTable->setItem(row, 2, new QTableWidgetItem(config.createdDate));
            projectTable->setItem(row, 3, new QTableWidgetItem(config.projectPath));
        }
    }
}

QStringList OpenProjectDialog::getSelectedPaths() const
{
    QStringList paths;
    QModelIndexList selectedRows = projectTable->selectionModel()->selectedRows();

    for (const QModelIndex &index : selectedRows) {
        QString path = projectTable->item(index.row(), 3)->text();
        if (!path.isEmpty()) {
            paths.append(path);
        }
    }
    return paths;
}

void OpenProjectDialog::applyStyles()
{
    // 复用 NewProjectDialog 的样式，保持统一
    this->setStyleSheet(R"(
        QDialog { background-color: #FFFFFF; }
        
        /* 1. 顶部标签（如"工程位置"、"工艺筛选"）的字体 */
        QLabel { font-size: 18px; color: #333333; font-weight: bold; }
        
        /* 2. 输入框和下拉框的字体 */
        QLineEdit, QComboBox {
            border: 1px solid #CCCCCC; border-radius: 4px; padding: 5px; background: #FAFAFA; font-size: 16px;
        }
        
        /* 3. 【核心修改】表格内内容的字体（工程名称、工艺类型等） */
        QTableWidget {
            border: 1px solid #dcdcdc;
            gridline-color: #eeeeee;
            selection-background-color: #e6f7ff;
            selection-color: #000000;
            font-size: 16px;
        }
        
        /* 4. 【核心修改】表头标题的字体（工程名称、工艺类型等四个标题） */
        QHeaderView::section {
            background-color: #f5f7fa;
            border: none;
            border-bottom: 1px solid #dcdcdc;
            padding: 8px;
            font-weight: bold;
            font-size: 18px;
        }
        
        /* 5. 底部按钮字体同步调大 */
        QPushButton {
            border: 1px solid #CCCCCC; border-radius: 4px; background-color: #F0F0F0; padding: 6px 15px; font-size: 16px;
        }
        QPushButton:hover { background-color: #E0E0E0; }

        QPushButton#OpenBtn { background-color: #0078D7; color: white; border: none; }
        QPushButton#OpenBtn:hover { background-color: #1084E3; }
        QPushButton#OpenBtn:disabled { background-color: #CCCCCC; color: #EEEEEE; }
    )");
}
