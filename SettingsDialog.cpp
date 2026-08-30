#include "SettingsDialog.h"

#include "AppInfo.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

namespace {
const QString CONFIG_GROUP = QStringLiteral("Solver");
const QString KEY_ABAQUS_PATH = QStringLiteral("AbaqusPath");
const QString DEFAULT_ABAQUS_PATH = QStringLiteral("C:/SIMULIA/Commands/abaqus.bat");
}

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("系统环境配置"));
    setMinimumSize(960, 280);
    resize(960, 280);
    setModal(true);
    setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
            font-family: 'Microsoft YaHei';
            font-size: 15px;
        }

        QLabel {
            font-size: 16px;
            font-weight: bold;
        }

        QLineEdit {
            font-size: 15px;
        }

        QPushButton {
            font-family: 'Microsoft YaHei';
            font-size: 15px;
        }

        QGroupBox {
            font-family: 'Microsoft YaHei';
            font-weight: bold;
            font-size: 16px;
            border: 1px solid #ccc;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 12px;
        }

        QGroupBox::title {
            left: 10px;
            padding: 0 5px;
        }
    )");
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    QGroupBox *solverGroup = new QGroupBox(QStringLiteral("求解器配置"), this);
    QHBoxLayout *groupLayout = new QHBoxLayout(solverGroup);
    groupLayout->setContentsMargins(16, 16, 16, 16);
    groupLayout->setSpacing(8);

    QLabel *pathLabel = new QLabel(QStringLiteral("Abaqus路径:"), this);
    pathLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    pathLabel->setFixedWidth(100);

    m_exePathEdit = new QLineEdit(this);
    m_exePathEdit->setPlaceholderText(QStringLiteral("例如：C:\\SIMULIA\\Commands\\abaqus.bat"));
    m_exePathEdit->setClearButtonEnabled(true);
    m_exePathEdit->setFixedHeight(32);

    m_browseExeBtn = new QPushButton(QStringLiteral("浏览"), this);
    m_browseExeBtn->setFixedSize(100, 36);

    groupLayout->addWidget(pathLabel);
    groupLayout->addWidget(m_exePathEdit, 1);
    groupLayout->addWidget(m_browseExeBtn);

    mainLayout->addWidget(solverGroup);
    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_saveBtn = new QPushButton(QStringLiteral("保存"), this);
    m_saveBtn->setFixedSize(100, 36);
    m_saveBtn->setDefault(true);
    btnLayout->addWidget(m_saveBtn);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setFixedSize(100, 36);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_browseExeBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseExe);
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::loadSettings()
{
    const QString path = SettingsDialog::getAbaqusPath();
    const QString displayPath = path.isEmpty() ? DEFAULT_ABAQUS_PATH : path;
    m_exePathEdit->setText(displayPath);
    m_exePathEdit->setCursorPosition(0);
    m_exePathEdit->setToolTip(displayPath);

    // 按路径文字宽度加宽窗口，打开时尽量完整显示
    const int textWidth = m_exePathEdit->fontMetrics().horizontalAdvance(displayPath);
    const int chromeWidth = 24 * 2 + 16 * 2 + 100 + 100 + 8 * 2 + 48;
    const int desiredWidth = qBound(chromeWidth + textWidth, 960, 1400);
    resize(desiredWidth, height());
}

QString SettingsDialog::getAbaqusPath()
{
    QSettings settings(
        AppInfo::OrganizationName,
        QStringLiteral("Settings")
    );
    settings.beginGroup(CONFIG_GROUP);
    const QString path = settings.value(KEY_ABAQUS_PATH).toString();
    settings.endGroup();
    return path.isEmpty() ? DEFAULT_ABAQUS_PATH : path;
}

void SettingsDialog::onBrowseExe()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择 Abaqus 启动程序"),
        QFileInfo(m_exePathEdit->text().trimmed()).absolutePath(),
        QStringLiteral("Abaqus 启动脚本 (*.bat *.cmd);;All Files (*.*)")
    );

    if (!filePath.isEmpty()) {
        m_exePathEdit->setText(filePath);
        m_exePathEdit->setCursorPosition(0);
        m_exePathEdit->setToolTip(filePath);

        const int textWidth = m_exePathEdit->fontMetrics().horizontalAdvance(filePath);
        const int chromeWidth = 24 * 2 + 16 * 2 + 100 + 100 + 8 * 2 + 48;
        const int desiredWidth = qBound(chromeWidth + textWidth, 960, 1400);
        resize(desiredWidth, height());
    }
}

void SettingsDialog::onSave()
{
    const QString path = m_exePathEdit->text().trimmed();
    const QFileInfo pathInfo(path);
    if (path.isEmpty()
        || !pathInfo.exists()
        || !pathInfo.isFile()) {
        QMessageBox msgBox(
            QMessageBox::Critical,
            QStringLiteral("错误"),
            QStringLiteral(
                "填写的 Abaqus 路径无效，"
                "必须指向实际启动文件！"
            ),
            QMessageBox::Ok,
            this
        );
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.exec();
        return;
    }

    QSettings settings(
        AppInfo::OrganizationName,
        QStringLiteral("Settings")
    );
    settings.beginGroup(CONFIG_GROUP);
    settings.setValue(KEY_ABAQUS_PATH, path);
    settings.endGroup();

    accept();
}
