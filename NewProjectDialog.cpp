#include "NewProjectDialog.h"

#include "ProjectManager.h"

#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QVBoxLayout>

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("新建工程"));
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    resize(500, 280);
    setFixedSize(500, 280);

    setupUi();
    applyStyles();
}

void NewProjectDialog::accept()
{
    QSettings settings(QStringLiteral("PBXSimulationSoftware"), QStringLiteral("NewProjectDialog"));
    settings.setValue(QStringLiteral("lastProjectPath"), pathEdit->text().trimmed());
    QDialog::accept();
}

void NewProjectDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    titleLabel = new QLabel(QStringLiteral("浇注XX固化过程仿真分析与三维参数重构分析软件"));
    titleLabel->setObjectName(QStringLiteral("HeaderLabel"));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QGridLayout *formLayout = new QGridLayout();
    formLayout->setVerticalSpacing(20);
    formLayout->setHorizontalSpacing(10);

    QLabel *nameLabel = new QLabel(QStringLiteral("工程名称:"));
    nameLabel->setObjectName(QStringLiteral("FormLabel"));
    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(QStringLiteral("例如：PBX_Test_01"));
    nameEdit->setClearButtonEnabled(true);

    QLabel *pathLabel = new QLabel(QStringLiteral("存储位置:"));
    pathLabel->setObjectName(QStringLiteral("FormLabel"));
    pathEdit = new QLineEdit();
    pathEdit->setPlaceholderText(QStringLiteral("选择工程保存的文件夹..."));

    QSettings settings(QStringLiteral("PBXSimulationSoftware"), QStringLiteral("NewProjectDialog"));
    const QString lastPath = settings.value(QStringLiteral("lastProjectPath"), QDir::homePath()).toString();
    pathEdit->setText(lastPath);

    browseBtn = new QPushButton(QStringLiteral("浏览"));
    browseBtn->setObjectName(QStringLiteral("BrowseBtn"));
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setFixedSize(100, 36);

    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(nameEdit, 0, 1, 1, 2);
    formLayout->addWidget(pathLabel, 1, 0);
    formLayout->addWidget(pathEdit, 1, 1);
    formLayout->addWidget(browseBtn, 1, 2);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(15);
    btnLayout->addStretch();

    cancelBtn = new QPushButton(QStringLiteral("取消"));
    cancelBtn->setObjectName(QStringLiteral("CancelBtn"));
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedSize(100, 36);

    okBtn = new QPushButton(QStringLiteral("立即创建"));
    okBtn->setObjectName(QStringLiteral("OkBtn"));
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setFixedSize(120, 36);
    okBtn->setEnabled(false);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);
    mainLayout->addLayout(btnLayout);

    connect(browseBtn, &QPushButton::clicked, this, &NewProjectDialog::onBrowsePath);
    connect(okBtn, &QPushButton::clicked, this, &NewProjectDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateOkButtonState(); });
    connect(pathEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateOkButtonState(); });
}

void NewProjectDialog::updateOkButtonState()
{
    const QString name = nameEdit->text().trimmed();
    const bool hasPath = !pathEdit->text().trimmed().isEmpty();

    QString nameError;
    const bool validName =
        !name.isEmpty()
        && ProjectManager::isValidProjectName(name, nameError);

    okBtn->setEnabled(validName && hasPath);
}

QString NewProjectDialog::getProjectName() const
{
    return nameEdit->text().trimmed();
}

QString NewProjectDialog::getProjectPath() const
{
    return pathEdit->text().trimmed();
}

void NewProjectDialog::onBrowsePath()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择工程路径"), pathEdit->text());
    if (!dir.isEmpty()) {
        pathEdit->setText(dir);
        updateOkButtonState();
    }
}

void NewProjectDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #FFFFFF;
        }
        QLabel {
            font-family: 'Microsoft YaHei', 'Segoe UI';
            font-size: 16px;
            color: #333333;
        }
        QLabel#HeaderLabel {
            font-size: 24px;
            font-weight: bold;
            color: #000000;
            padding: 10px 0px;
            border-bottom: 1px solid #EEEEEE;
            margin-bottom: 10px;
        }
        QLabel#FormLabel {
            font-weight: bold;
        }
        QLineEdit {
            border: 1px solid #CCCCCC;
            border-radius: 4px;
            padding: 6px 10px;
            min-height: 20px;
            font-family: 'Microsoft YaHei';
            font-size: 15px;
            background-color: #FAFAFA;
        }
        QLineEdit:focus {
            border: 1px solid #0078D7;
            background-color: #FFFFFF;
        }
        QPushButton {
            border: 1px solid #CCCCCC;
            border-radius: 4px;
            background-color: #F0F0F0;
            color: #333333;
            font-family: 'Microsoft YaHei';
            font-size: 14px;
            padding: 6px 15px;
        }
        QPushButton:hover {
            background-color: #E0E0E0;
            border-color: #999999;
        }
        QPushButton:pressed {
            background-color: #D0D0D0;
        }
        QPushButton#OkBtn {
            background-color: #0078D7;
            color: white;
            border: none;
        }
        QPushButton#OkBtn:hover {
            background-color: #1084E3;
        }
        QPushButton#OkBtn:pressed {
            background-color: #006CC1;
        }
        QPushButton#OkBtn:disabled {
            background-color: #CCCCCC;
            color: #EEEEEE;
        }
        QPushButton#CancelBtn:hover {
            color: #d81e06;
            border-color: #d81e06;
        }
    )");
}
