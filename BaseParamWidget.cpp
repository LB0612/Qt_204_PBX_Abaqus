#include "BaseParamWidget.h" 
#include <QHBoxLayout> 
#include <QPushButton> 
#include <QScrollArea> 
#include <QDoubleValidator>

BaseParamWidget::BaseParamWidget(QWidget *parent) : QWidget(parent) {} 
BaseParamWidget::~BaseParamWidget() {} 

// ================================================================= 
// 1. 添加参数行 (高度增加至 40px，更舒展) 
// ================================================================= 
void BaseParamWidget::addParamRow(QBoxLayout* layout, const QString& name, QLineEdit* edit, const QString& unit) 
{ 
    QHBoxLayout *hLayout = new QHBoxLayout(); 
    
    // 标签：240px宽，右对齐
    QString displayName = name;
    if (!unit.isEmpty()) {
        displayName = name + QStringLiteral("（") + unit + QStringLiteral("）");
    }

    QLabel *lblName = new QLabel(displayName + QStringLiteral("："));
    lblName->setFixedWidth(240); 
    lblName->setAlignment(Qt::AlignRight | Qt::AlignVCenter); 
    // 【修改】高度改为 40 
    lblName->setMinimumHeight(40); 
    // 字体保持微软雅黑黑体 
    lblName->setStyleSheet("font-family: 'Microsoft YaHei'; font-size: 16px; color: #000000; font-weight: bold;"); 
    
    hLayout->addWidget(lblName); 
    
    // 【修改】输入框高度也改为 40 
    edit->setMinimumHeight(40); 
    edit->setStyleSheet("QLineEdit { font-family: 'Microsoft YaHei'; font-size: 16px; margin-left: 10px; padding-left: 5px; border: 1px solid #ccc; border-radius: 4px; } QLineEdit:focus { border: 1px solid #1890ff; }"); 
    
    hLayout->addWidget(edit);

    connect(
        edit,
        &QLineEdit::textEdited,
        this,
        &BaseParamWidget::parameterEdited
    );

    hLayout->setContentsMargins(0, 5, 0, 5);
    layout->addLayout(hLayout);

    m_paramEdits.append(edit);
}

void BaseParamWidget::applyCommonStyles() { 
    this->setStyleSheet("background-color: white;"); 
} 

QVBoxLayout* BaseParamWidget::createMainLayout(QWidget* parent) { 
    QVBoxLayout* layout = new QVBoxLayout(parent); 
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0); 
    return layout; 
} 

QScrollArea* BaseParamWidget::createScrollArea(QWidget* parent) { 
    QScrollArea* scroll = new QScrollArea(parent); 
    scroll->setWidgetResizable(true); 
    scroll->setFrameShape(QFrame::NoFrame); 
    return scroll; 
} 

QVBoxLayout* BaseParamWidget::createScrollContentLayout(QWidget* contentWidget) { 
    QVBoxLayout* layout = new QVBoxLayout(contentWidget); 
    // 边距 
    layout->setContentsMargins(30, 20, 30, 40); 
    layout->setSpacing(10); 
    return layout; 
} 

void BaseParamWidget::setupHeader(const QString& title) { 
    QWidget* header = new QWidget(); 
    header->setFixedHeight(60); 
    header->setStyleSheet("background-color: #f8f9fa; border-bottom: 1px solid #ddd;"); 
    
    QHBoxLayout* hLayout = new QHBoxLayout(header); 
    hLayout->setContentsMargins(20, 0, 20, 0); 
    
    QLabel* lbl = new QLabel(title); 
    lbl->setStyleSheet("font-family: 'Microsoft YaHei'; font-size: 20px; font-weight: bold; color: #333;"); 
    hLayout->addWidget(lbl); 
    
    hLayout->addStretch(); 
    
    QPushButton* btnBack = new QPushButton("返回首页"); 
    btnBack->setFixedSize(100, 36); 
    btnBack->setCursor(Qt::PointingHandCursor); 
    btnBack->setStyleSheet(
        "QPushButton { background-color: #e6e6e6; color: #333; border: 1px solid #ccc; border-radius: 4px; font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 16px; } "
        "QPushButton:hover { background-color: #d4d4d4; } "
        "QPushButton:pressed { background-color: #c0c0c0; }"
    ); 
    connect(btnBack, &QPushButton::clicked, this, &BaseParamWidget::backClicked); 
    hLayout->addWidget(btnBack); 
    
    this->layout()->addWidget(header); 
} 

void BaseParamWidget::addSaveButton(QVBoxLayout* layout, const QString& text, std::function<void()> onClick) {
    QPushButton* btn = new QPushButton(text);
    btn->setFixedHeight(60);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton { background-color: #007bff; color: white; border-radius: 5px; font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 18px; margin-top: 25px; } "
        "QPushButton:hover { background-color: #0069d9; } "
        "QPushButton:disabled {"
        " background-color: #bfbfbf;"
        " color: #f5f5f5;"
        "}"
    );
    connect(btn, &QPushButton::clicked, this, onClick);

    m_saveButtons.append(btn);

    layout->addWidget(btn);
} 

QLineEdit* BaseParamWidget::createSciEdit(const QString &text) { 
    QLineEdit *edit = new QLineEdit(text);
    // 允许科学计数法输入，例如 1.23E-4
    QDoubleValidator *validator = new QDoubleValidator(edit);
    validator->setNotation(QDoubleValidator::ScientificNotation);
    edit->setValidator(validator);
    return edit;
} 

QLineEdit* BaseParamWidget::createReadOnlyEdit() { 
    QLineEdit* edit = new QLineEdit(); 
    edit->setReadOnly(true); 
    // 只读框高度也加大 
    edit->setMinimumHeight(40); 
    edit->setStyleSheet("QLineEdit { font-family: 'Microsoft YaHei'; font-size: 16px; margin-left: 10px; padding-left: 5px; border: 1px solid #eee; background-color: #f9f9f9; border-radius: 4px; color: #555; }"); 
    return edit; 
}

void BaseParamWidget::setReadOnlyMode(bool readOnly)
{
    for (QLineEdit *edit : m_paramEdits) {
        if (edit) {
            edit->setReadOnly(readOnly);
        }
    }

    for (QPushButton *button : m_saveButtons) {
        if (button) {
            button->setEnabled(!readOnly);
        }
    }
}
