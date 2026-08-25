#ifndef SIMULATIONPREPAREWIDGET_H
#define SIMULATIONPREPAREWIDGET_H

#include "BaseParamWidget.h"

class QPushButton;
class QLabel;
class QFrame;

class SimulationPrepareWidget : public BaseParamWidget
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

private:
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
};

#endif
