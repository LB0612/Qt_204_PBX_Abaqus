#ifndef SIMULATIONMONITORWIDGET_H
#define SIMULATIONMONITORWIDGET_H

#include "BaseParamWidget.h"

class QLabel;
class QTextEdit;
class QProgressBar;

class SimulationMonitorWidget : public BaseParamWidget
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
    QLabel *statusValueLabel = nullptr;
    QProgressBar *progressBar = nullptr;
    QTextEdit *logEdit = nullptr;
};

#endif
