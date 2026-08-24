#ifndef BASEPARAMWIDGET_H
#define BASEPARAMWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include <QScrollArea>
#include <QPushButton>
#include <functional>

class BaseParamWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseParamWidget(QWidget *parent = nullptr);
    virtual ~BaseParamWidget();

    void setReadOnlyMode(bool readOnly);

protected:
    void addParamRow(QBoxLayout* layout, const QString& name, QLineEdit* edit, const QString& unit = "");
    void addSaveButton(QVBoxLayout* layout, const QString& text, std::function<void()> onClick);

    QVBoxLayout* createMainLayout(QWidget* parent);
    QScrollArea* createScrollArea(QWidget* parent);
    QVBoxLayout* createScrollContentLayout(QWidget* contentWidget);
    void setupHeader(const QString& title);

    QLineEdit* createSciEdit(const QString &text = "");
    QLineEdit* createReadOnlyEdit();
    void applyCommonStyles();

    QList<QLineEdit *> m_paramEdits;
    QList<QPushButton *> m_saveButtons;

signals:
    void backClicked();
    void parameterEdited();
};

#endif // BASEPARAMWIDGET_H
