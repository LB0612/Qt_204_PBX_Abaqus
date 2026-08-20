#include "SettingsDialog.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
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
    setMinimumSize(700, 220);
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
    )");
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(20);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(16);

    m_exePathEdit = new QLineEdit(this);
    m_exePathEdit->setPlaceholderText(QStringLiteral("例如：C:\\SIMULIA\\Commands\\abaqus.bat"));
    m_exePathEdit->setClearButtonEnabled(true);
    m_exePathEdit->setMinimumHeight(32);

    m_browseExeBtn = new QPushButton(QStringLiteral("浏览"), this);
    m_browseExeBtn->setFixedSize(80, 32);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->setSpacing(8);
    pathLayout->addWidget(m_exePathEdit, 1);
    pathLayout->addWidget(m_browseExeBtn);

    formLayout->addRow(QStringLiteral("Abaqus启动程序："), pathLayout);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setFixedSize(100, 32);
    btnLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton(QStringLiteral("保存"), this);
    m_saveBtn->setFixedSize(100, 32);
    m_saveBtn->setDefault(true);
    btnLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_browseExeBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseExe);
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::loadSettings()
{
    const QString path = SettingsDialog::getAbaqusPath();
    m_exePathEdit->setText(path.isEmpty() ? DEFAULT_ABAQUS_PATH : path);
}

QString SettingsDialog::getAbaqusPath()
{
    QSettings settings(QStringLiteral("PBXSimulationSoftware"), QStringLiteral("Settings"));
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
    }
}

void SettingsDialog::onSave()
{
    const QString path = m_exePathEdit->text().trimmed();
    if (path.isEmpty() || !QFile::exists(path)) {
        QMessageBox msgBox(
            QMessageBox::Critical,
            QStringLiteral("错误"),
            QStringLiteral("填写的路径无效或文件不存在！"),
            QMessageBox::Ok,
            this
        );
        msgBox.setWindowModality(Qt::ApplicationModal);
        msgBox.exec();
        return;
    }

    QSettings settings(QStringLiteral("PBXSimulationSoftware"), QStringLiteral("Settings"));
    settings.beginGroup(CONFIG_GROUP);
    settings.setValue(KEY_ABAQUS_PATH, path);
    settings.endGroup();

    accept();
}
