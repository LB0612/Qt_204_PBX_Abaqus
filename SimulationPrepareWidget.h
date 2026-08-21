#ifndef SIMULATIONPREPAREWIDGET_H
#define SIMULATIONPREPAREWIDGET_H

#include <QWidget>

class QPushButton;
class QLabel;

class SimulationPrepareWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimulationPrepareWidget(QWidget *parent = nullptr);

signals:
    void startRequested();
    void cancelRequested();

private:
    QLabel *titleLabel;
    QLabel *infoLabel;
    QPushButton *startButton;
    QPushButton *cancelButton;
};

#endif
