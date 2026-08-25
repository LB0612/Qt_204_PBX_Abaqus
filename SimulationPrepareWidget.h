#ifndef SIMULATIONPREPAREWIDGET_H
#define SIMULATIONPREPAREWIDGET_H

#include <QWidget>

class QPushButton;
class QLabel;
class QFrame;

class SimulationPrepareWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimulationPrepareWidget(QWidget *parent = nullptr);

    void setReadyState(
        bool ready,
        const QString &message = QString()
    );

signals:
    void startRequested();
    void cancelRequested();

private:
    QLabel *titleLabel = nullptr;
    QFrame *statusCard = nullptr;
    QLabel *statusTitleLabel = nullptr;
    QLabel *checkParamLabel = nullptr;
    QLabel *checkFilesLabel = nullptr;
    QLabel *checkPathLabel = nullptr;
    QLabel *hintLabel = nullptr;
    QLabel *errorStatusLabel = nullptr;
    QLabel *reasonTitleLabel = nullptr;
    QLabel *reasonContentLabel = nullptr;
    QPushButton *startButton = nullptr;
    QPushButton *cancelButton = nullptr;
};

#endif
