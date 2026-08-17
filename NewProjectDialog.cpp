#include "NewProjectDialog.h"
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSettings>

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("新建工程向导");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    resize(500, 320);
    setFixedSize(500, 320);

    setupUi();
    applyStyles();
}

void NewProjectDialog::accept()
{
    // 保存当前选择的路径
    QSettings settings("SimulationSoftware", "NewProjectDialog");
    settings.setValue("lastProjectPath", pathEdit->text().trimmed());
    
    QDialog::accept();
}

void NewProjectDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    titleLabel = new QLabel("创建新仿真工程");
    titleLabel->setObjectName("HeaderLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    QGridLayout *formLayout = new QGridLayout();
    formLayout->setVerticalSpacing(20);
    formLayout->setHorizontalSpacing(10);

    QLabel *nameLabel = new QLabel("工程名称:");
    nameLabel->setObjectName("FormLabel");
    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("例如：Project_2024_001");
    nameEdit->setClearButtonEnabled(true);

    QLabel *pathLabel = new QLabel("存储位置:");
    pathLabel->setObjectName("FormLabel");
    pathEdit = new QLineEdit();
    pathEdit->setPlaceholderText("选择工程保存的文件夹...");
    
    // 读取上一次保存的路径
    QSettings settings("SimulationSoftware", "NewProjectDialog");
    QString lastPath = settings.value("lastProjectPath", QDir::homePath()).toString();
    pathEdit->setText(lastPath);
    pathEdit->setReadOnly(false);

    browseBtn = new QPushButton("浏览");
    browseBtn->setObjectName("BrowseBtn");
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setFixedSize(100, 36);

    QLabel *typeLabel = new QLabel("工艺类型:");
    typeLabel->setObjectName("FormLabel");
    typeCombo = new QComboBox();
    typeCombo->addItems({"捏合混匀工艺 (Kneading)", "挤压造粒工艺 (Extrusion)", "压制成型工艺 (Pressing)"});
    typeCombo->setCursor(Qt::PointingHandCursor);

    formLayout->addWidget(nameLabel, 0, 0);
    formLayout->addWidget(nameEdit, 0, 1, 1, 2);

    formLayout->addWidget(pathLabel, 1, 0);
    formLayout->addWidget(pathEdit, 1, 1);
    formLayout->addWidget(browseBtn, 1, 2);

    formLayout->addWidget(typeLabel, 2, 0);
    formLayout->addWidget(typeCombo, 2, 1, 1, 2);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(15);
    btnLayout->addStretch();

    cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("CancelBtn");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedSize(90, 32);

    okBtn = new QPushButton("立即创建");
    okBtn->setObjectName("OkBtn");
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setFixedSize(100, 32);
    okBtn->setEnabled(false);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(okBtn);

    mainLayout->addLayout(btnLayout);

    connect(browseBtn, &QPushButton::clicked, this, &NewProjectDialog::onBrowsePath);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool hasName = !text.trimmed().isEmpty();
        bool hasPath = !pathEdit->text().trimmed().isEmpty();
        okBtn->setEnabled(hasName && hasPath);
    });
}

QString NewProjectDialog::getProjectName() const
{
    return nameEdit->text().trimmed();
}

QString NewProjectDialog::getProjectPath() const
{
    return pathEdit->text().trimmed();
}

int NewProjectDialog::getProcessType() const
{
    return typeCombo->currentIndex();
}

QString NewProjectDialog::getTypeName() const
{
    return typeCombo->currentText();
}

void NewProjectDialog::onBrowsePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择工程路径", pathEdit->text());
    if (!dir.isEmpty()) {
        pathEdit->setText(dir);
        emit nameEdit->textChanged(nameEdit->text());
    }
}

void NewProjectDialog::applyStyles()
{
    QString qss = R"(
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
            font-size: 15px;
            background-color: #FAFAFA;
        }
        QLineEdit:focus {
            border: 1px solid #0078D7;
            background-color: #FFFFFF;
        }

        QComboBox {
            border: 1px solid #CCCCCC;
            border-radius: 4px;
            padding: 6px 10px;
            min-height: 20px;
            font-size: 15px;
            background-color: #FAFAFA;
        }
        QComboBox:hover {
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
    )";

    this->setStyleSheet(qss);
}
