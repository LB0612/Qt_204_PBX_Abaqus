#include "SettingsDialog.h" 
#include <QVBoxLayout> 
#include <QHBoxLayout> 
#include <QGroupBox> 
#include <QFileDialog> 
#include <QSettings> 
#include <QMessageBox> 
#include <QFileInfo> 
#include <QDir> 
#include <QFile> 
#include <QFormLayout> 
#include <QSpacerItem> 
#include <QSizePolicy> 

// 配置存储常量 
const QString CONFIG_GROUP = "Solver"; 
const QString KEY_POLYFLOW_PATH = "PolyflowPath"; 

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) { 
    setWindowTitle("系统环境配置"); 
    setMinimumSize(700, 280); // 设置最小尺寸 
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 允许水平扩展 
    setModal(true); 
    setupUi(); 
    loadSettings(); 
} 

void SettingsDialog::setupUi() { 
    // 设置整体字体大小 
    QFont defaultFont = this->font(); 
    defaultFont.setPointSize(defaultFont.pointSize() + 1); // 整体字体增大1pt 
    this->setFont(defaultFont); 
    
    // 创建路径区域和按钮的字体，再增大1pt，总共增大2pt（微妙增量）
    QFont pathFont = defaultFont; 
    pathFont.setPointSize(pathFont.pointSize() + 1); // 路径区域字体再增大1pt 
    
    QFont buttonFont = defaultFont; 
    buttonFont.setPointSize(buttonFont.pointSize() + 1); // 按钮字体再增大1pt 
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this); 
    mainLayout->setContentsMargins(24, 24, 24, 24); // 调整边距为24px，更符合现代UI规范 
    mainLayout->setSpacing(20); // 调整控件间距为20px 

    // 创建求解器配置组 
    QGroupBox *solverGroup = new QGroupBox("求解器配置", this); 
    // 将求解器配置标题字体设置为与polyflow路径相同的字体 
    solverGroup->setFont(pathFont); 
    
    // 使用水平布局组织所有核心元素 
    QHBoxLayout *groupLayout = new QHBoxLayout(solverGroup); 
    groupLayout->setContentsMargins(16, 16, 16, 16); 
    groupLayout->setSpacing(8); // 控件间距8px，保持紧凑 
    
    // 1. 简化后的Polyflow编辑器路径显示区 - 精简的标签 
    QLabel *pathLabel = new QLabel("Polyflow路径:", this); 
    pathLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight); // 垂直居中，右对齐 
    pathLabel->setFixedWidth(100); // 增加宽度，确保冒号可见 
    pathLabel->setFont(pathFont); // 应用增大的路径字体 
    pathLabel->setStyleSheet("color: #333333; font-weight: medium;"); // 加深颜色，增强对比度 
    groupLayout->addWidget(pathLabel); 
    
    // 2. 主编辑框区域 
    m_exePathEdit = new QLineEdit(this); 
    m_exePathEdit->setPlaceholderText("等待设置路径..."); 
    m_exePathEdit->setFixedHeight(32); // 输入框高度32px，更紧凑 
    m_exePathEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // 输入框可水平扩展 
    m_exePathEdit->setReadOnly(false); // 允许用户编辑 
    m_exePathEdit->setClearButtonEnabled(true); // 显示清除按钮，方便用户清空内容 
    m_exePathEdit->setFont(pathFont); // 应用增大的路径字体 
    
    // 连接文本变化信号，实现动态宽度调整 
    connect(m_exePathEdit, &QLineEdit::textChanged, this, [this]() {
        // 使用QFontMetrics计算文本宽度
        QFontMetrics metrics(m_exePathEdit->font());
        int textWidth = metrics.horizontalAdvance(m_exePathEdit->text());
        // 添加一些内边距，确保文本完全可见
        int editMinWidth = textWidth + 40; // 40px 额外空间用于清除按钮和内边距
        // 设置QLineEdit的最小宽度
        m_exePathEdit->setMinimumWidth(editMinWidth);
        
        // 计算对话框所需的最小宽度（包含所有控件和边距）
        int fixedWidths = 100; // 标签固定宽度（已调整为100px）
        fixedWidths += 90 + 80; // 两个按钮的固定宽度
        fixedWidths += 8 * 3; // 控件间距
        fixedWidths += 24 * 2 + 16 * 2; // 对话框和分组框的边距
        int dialogMinWidth = fixedWidths + editMinWidth;
        // 设置对话框的最小宽度，确保所有内容都能完整显示
        this->setMinimumWidth(dialogMinWidth);
    });
    
    groupLayout->addWidget(m_exePathEdit, 1); // 输入框占据主要空间 
    
    // 3. 智能扫描功能按钮 
    m_autoScanBtn = new QPushButton("智能扫描", this); 
    m_autoScanBtn->setFixedHeight(32); // 统一按钮高度 
    m_autoScanBtn->setFixedWidth(90); // 固定宽度，保持紧凑 
    m_autoScanBtn->setFont(buttonFont); // 应用增大的按钮字体 
    groupLayout->addWidget(m_autoScanBtn); 
    
    // 4. 文件浏览功能按钮 
    m_browseExeBtn = new QPushButton("浏览", this); 
    m_browseExeBtn->setFixedHeight(32); // 统一按钮高度 
    m_browseExeBtn->setFixedWidth(80); // 固定宽度，保持紧凑 
    m_browseExeBtn->setFont(buttonFont); // 应用增大的按钮字体 
    groupLayout->addWidget(m_browseExeBtn); 
    
    mainLayout->addWidget(solverGroup, 0); // 求解器配置组不扩展 

    // 状态标签 
    m_statusLabel = new QLabel(" ", this); 
    m_statusLabel->setStyleSheet("color: #666666; font-size: 12px;"); 
    m_statusLabel->setAlignment(Qt::AlignLeft); 
    mainLayout->addWidget(m_statusLabel); 

    // 按钮布局区域 
    QHBoxLayout *btnLayout = new QHBoxLayout(); 
    btnLayout->setSpacing(12); // 按钮间距12px 
    btnLayout->addStretch(1); // 左侧拉伸，将按钮推到右侧 
    
    m_saveBtn = new QPushButton("保存", this); 
    m_saveBtn->setFixedHeight(32); // 统一按钮高度 
    m_saveBtn->setMinimumWidth(100); // 设置最小宽度 
    m_saveBtn->setDefault(true); // 设置为默认按钮 
    m_saveBtn->setFont(buttonFont); // 应用增大的按钮字体 
    btnLayout->addWidget(m_saveBtn); 
    
    m_cancelBtn = new QPushButton("取消", this); 
    m_cancelBtn->setFixedHeight(32); // 统一按钮高度 
    m_cancelBtn->setMinimumWidth(100); // 设置最小宽度 
    m_cancelBtn->setFont(buttonFont); // 应用增大的按钮字体 
    btnLayout->addWidget(m_cancelBtn); 
    
    mainLayout->addLayout(btnLayout); 

    // 连接信号槽 
    connect(m_autoScanBtn, &QPushButton::clicked, this, &SettingsDialog::onAutoScan); 
    connect(m_browseExeBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseExe); 
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave); 
    connect(m_cancelBtn, &QPushButton::clicked, this, &SettingsDialog::reject); 
} 

void SettingsDialog::loadSettings() { 
    m_exePathEdit->setText(QDir::toNativeSeparators(SettingsDialog::getPolyflowPath())); 
    
    // 初始化时计算并设置合适的宽度
    QFontMetrics metrics(m_exePathEdit->font());
    int textWidth = metrics.horizontalAdvance(m_exePathEdit->text());
    int editMinWidth = textWidth + 40; // 40px 额外空间用于清除按钮和内边距
    m_exePathEdit->setMinimumWidth(editMinWidth);
    
    // 计算对话框所需的最小宽度（包含所有控件和边距）
    int fixedWidths = 100; // 标签固定宽度（已调整为100px）
    fixedWidths += 90 + 80; // 两个按钮的固定宽度
    fixedWidths += 8 * 3; // 控件间距
    fixedWidths += 24 * 2 + 16 * 2; // 对话框和分组框的边距
    int dialogMinWidth = fixedWidths + editMinWidth;
    
    // 确保对话框最小宽度不小于默认值700px
    dialogMinWidth = qMax(dialogMinWidth, 700);
    
    // 设置对话框的最小宽度，确保所有内容都能完整显示
    this->setMinimumWidth(dialogMinWidth);
} 

QString SettingsDialog::getPolyflowPath() { 
    QSettings settings("MyCompany", "SimulationApp"); 
    settings.beginGroup(CONFIG_GROUP); 
    return settings.value(KEY_POLYFLOW_PATH).toString(); 
} 

void SettingsDialog::onBrowseExe() { 
    QString filePath = QFileDialog::getOpenFileName(this, "选择 Polyflow 求解器", "C:/", "Executables (polyflow.exe);;All Files (*.*)"); 
    if (!filePath.isEmpty()) { 
        m_exePathEdit->setText(QDir::toNativeSeparators(filePath)); 
        // 清除状态标签，保持一致性 
        m_statusLabel->clear(); 
    } 
} 

// --- 彻底简化的智能扫描逻辑 --- 
void SettingsDialog::onAutoScan() { 
    QString foundPath; 
    
    // 统一定义需要检查的“唯一正确”相对路径结构 
    auto checkTarget = [](const QString &rootDir) -> QString { 
        // 循环匹配版本号 v190 - v260 
        for (int v = 260; v >= 190; --v) { 
            // 只看你指定的那个正确路径：ntbin/win64/polyflow.exe 
            QString testPath = QString("%1/v%2/polyflow/ntbin/win64/polyflow.exe").arg(rootDir).arg(v); 
            if (QFile::exists(testPath)) { 
                return testPath; 
            } 
        } 
        return QString(); 
    }; 

    // 1. 优先级一：直接去 C 盘的标准位置查找特定路径 
    foundPath = checkTarget("C:/Program Files/ANSYS Inc"); 

    // 2. 优先级二：C 盘没有，直接让用户告知安装目录（比如 D:/Ansys） 
    if (foundPath.isEmpty()) { 
        QMessageBox msgBox(QMessageBox::Information, "扫描提示", 
            "未在 C 盘默认位置找到求解器。\n请选择您的 ANSYS 安装目录，系统将在其对应的特定子目录下查找。", QMessageBox::Ok, this);
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.exec(); 
        
        QString userDir = QFileDialog::getExistingDirectory(this, "选择安装路径", "C:/"); 
        
        if (!userDir.isEmpty()) { 
            // 在用户指定的目录下，依然只查找那个特定的物理结构 
            foundPath = checkTarget(userDir); 
        } 
    } 

    // 3. 最终处理 
    if (!foundPath.isEmpty()) { 
        m_exePathEdit->setText(QDir::toNativeSeparators(foundPath)); 
        // 移除成功提示信息，只更新路径 
        m_statusLabel->clear(); 
    } else { 
        // 依然没找到，不再乱搜，直接提示用户手动浏览 
        QMessageBox msgBox(QMessageBox::Warning, "未找到", 
            "在指定路径下未发现符合结构的求解器。\n请点击\"浏览文件\"手动选择正确位置。", QMessageBox::Ok, this);
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.exec(); 
        m_statusLabel->setText("未找到文件"); 
    } 
} 

void SettingsDialog::onSave() { 
    QString path = m_exePathEdit->text().trimmed(); 
    if (path.isEmpty() || !QFile::exists(path)) { 
        QMessageBox msgBox(QMessageBox::Critical, "错误", "填写的路径无效或文件不存在！", QMessageBox::Ok, this);
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.exec(); 
        return; 
    } 
    QSettings settings("MyCompany", "SimulationApp"); 
    settings.beginGroup(CONFIG_GROUP); 
    settings.setValue(KEY_POLYFLOW_PATH, path); 
    settings.endGroup(); 
    accept(); 
}