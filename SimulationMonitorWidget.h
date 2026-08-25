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
    void clearLog();
    void setLogText(const QString &text);
    void setStatus(const QString &text);
    void setProgress(int value);

private:
    QFrame *createCard();

    QLabel *titleLabel;
    QLabel *statusValueLabel;
    QProgressBar *progressBar;
    QTextEdit *logEdit;
};

#endif
