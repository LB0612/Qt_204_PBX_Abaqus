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
    
    // 标签：180px宽，右对齐 
    QLabel *lblName = new QLabel(name + "："); 
    lblName->setFixedWidth(180); 
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
    
    if (!unit.isEmpty()) {
        QLabel *unitLabel = new QLabel(unit);

        unitLabel->setFixedWidth(80);

        unitLabel->setAlignment(
            Qt::AlignLeft |
            Qt::AlignVCenter
        );

        unitLabel->setStyleSheet(
            QStringLiteral(
                "font-family: 'Microsoft YaHei';"
                "font-size: 15px;"
                "color: #555;"
            )
        );

        hLayout->addWidget(unitLabel);
    }

    // 稍微增加一点行距 
    hLayout->setContentsMargins(0, 5, 0, 5); 
    layout->addLayout(hLayout); 
    
    // 注册
    QString cleanName = name;
    cleanName.remove(":"); cleanName.remove("：");
    cleanName = cleanName.trimmed(); // 去除可能存在的空格
    m_paramRegistry.append({cleanName, edit, unit, false}); 
} 

// ================================================================= 
// 2. 添加小标题 (高度增加至 120px，绝对不会切) 
// ================================================================= 
void BaseParamWidget::addSectionTitle(QBoxLayout* layout, const QString& title) 
{ 
    QLabel *lblTitle = new QLabel(title); 
    lblTitle->setAlignment(Qt::AlignCenter); 
    
    // 【核心修改】高度加大到 120px 
    // 120px 足够容纳最大的汉字和上下的边距，绝对不会再切了 
    lblTitle->setMinimumHeight(120); 
    lblTitle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed); 
    
    // 样式：黑体，20px 
    lblTitle->setStyleSheet("font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 20px; margin-top: 15px; margin-bottom: 5px; color: #000; border-bottom: 1px solid #eee; padding-bottom: 5px;"); 
    
    layout->addWidget(lblTitle); 
    
    m_paramRegistry.append({title, nullptr, "", true}); 
} 

// ================================================================= 
// 3. 辅助函数 
// ================================================================= 
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
    btn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border-radius: 5px; font-family: 'Microsoft YaHei'; font-weight: bold; font-size: 18px; margin-top: 25px; } QPushButton:hover { background-color: #0069d9; }");
    connect(btn, &QPushButton::clicked, this, onClick);
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
