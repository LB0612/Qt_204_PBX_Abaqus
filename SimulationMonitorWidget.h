#ifndef SIMULATIONMONITORWIDGET_H
#define SIMULATIONMONITORWIDGET_H

#include <QWidget>

class QLabel;
class QTextEdit;
class QProgressBar;
class QFrame;

class SimulationMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimulationMonitorWidget(QWidget *parent = nullptr);

public slots:
    void appendLog(const QString &text);
    void setStatus(const QString &text);
    void setPhase(const QString &text);
    void setJob(const QString &jobName);
    void setProgress(int value);

private:
    QFrame *createCard();

    QLabel *titleLabel;
    QLabel *statusValueLabel;
    QLabel *phaseValueLabel;
    QLabel *jobValueLabel;
    QProgressBar *progressBar;
    QTextEdit *logEdit;
};

#endif
