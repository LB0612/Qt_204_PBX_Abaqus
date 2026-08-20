#ifndef SIMULATIONMONITORWIDGET_H
#define SIMULATIONMONITORWIDGET_H

#include <QWidget>

class QLabel;
class QTextEdit;
class QProgressBar;

class SimulationMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimulationMonitorWidget(QWidget *parent = nullptr);

public slots:
    void appendLog(const QString &text);
    void setStatus(const QString &text);
    void setProgress(int value);

private:
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QTextEdit *logEdit;
};

#endif
