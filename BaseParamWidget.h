#ifndef BASEPARAMWIDGET_H
#define BASEPARAMWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QBoxLayout>
#include <QVBoxLayout> // 【修复】必须显式包含，因为下方用到了 QVBoxLayout*
#include <QList>
#include <QGroupBox>
#include <QScrollArea>
#include <QPushButton>
#include <functional> // 必须包含，用于 std::function

// 定义参数元数据结构
struct ParamInfo {
    QString label;      // 参数名
    QLineEdit* edit;    // 输入框指针
    QString unit;       // 单位
    bool isTitle;       // 是否是标题
};

class BaseParamWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseParamWidget(QWidget *parent = nullptr);
    virtual ~BaseParamWidget();

    void setReadOnlyMode(bool readOnly);

    // 通用接口：获取所有注册参数（供参数检查用）
    QList<ParamInfo> getRegisteredParams() const {
        return m_paramRegistry;
    }

protected:
    // --- 核心构建函数 ---
    void addParamRow(QBoxLayout* layout, const QString& name, QLineEdit* edit, const QString& unit = "");
    void addSectionTitle(QBoxLayout* layout, const QString& title);
    void addSaveButton(QVBoxLayout* layout, const QString& text, std::function<void()> onClick);

    // 【新增】手动注册参数（针对非 addParamRow 创建的场景）
    void registerParam(const QString& name, QLineEdit* edit, const QString& unit = "") {
        m_paramRegistry.append({name, edit, unit, false});
    }

    // --- 布局辅助函数 ---
    QVBoxLayout* createMainLayout(QWidget* parent);
    QScrollArea* createScrollArea(QWidget* parent);
    QVBoxLayout* createScrollContentLayout(QWidget* contentWidget);
    void setupHeader(const QString& title);
    
    // --- 控件工厂函数 ---
    QLineEdit* createSciEdit(const QString &text = "");
    QLineEdit* createReadOnlyEdit(); // 这个保留，工程信息页还在用
    void applyCommonStyles();

    // 参数注册表
    QList<ParamInfo> m_paramRegistry;
    QList<QPushButton *> m_saveButtons;

signals:
    void backClicked(); // 返回按钮信号
};

#endif // BASEPARAMWIDGET_H
