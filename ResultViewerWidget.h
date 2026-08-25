#ifndef RESULTVIEWERWIDGET_H
#define RESULTVIEWERWIDGET_H

#include "SimulationResultService.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;

class ResultViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResultViewerWidget(QWidget *parent = nullptr);

    void setProjectPath(const QString &projectPath);
    void refreshResults();
    void stopPlayback();

signals:
    void generateReportRequested();
    void openResultsDirectoryRequested();

private:
    void buildUi();
    void setControlsEnabled(bool enabled);
    void showMessage(const QString &text);
    void hideMessage();
    void switchResultType(ResultType type);
    void loadFrame(int frameIndex);
    void updateFrameInfo();
    void togglePlayback();
    void stepFrame(int delta);

    QString projectPath;
    ResultValidationResult validation;

    ResultType currentType = ResultType::Cure;
    int currentFrame = 0;
    int totalFrames = 0;
    int fps = 5;

    QTimer *playTimer = nullptr;

    QLabel *titleLabel = nullptr;
    QLabel *messageLabel = nullptr;
    QLabel *imageLabel = nullptr;
    QLabel *resultNameLabel = nullptr;
    QLabel *frameInfoLabel = nullptr;
    QLabel *simulationTimeLabel = nullptr;
    QLabel *metaLabel = nullptr;

    QPushButton *cureButton = nullptr;
    QPushButton *temperatureButton = nullptr;
    QPushButton *stressButton = nullptr;
    QPushButton *prevButton = nullptr;
    QPushButton *playButton = nullptr;
    QPushButton *nextButton = nullptr;
    QPushButton *openDirButton = nullptr;
    QPushButton *reportButton = nullptr;

    QSlider *frameSlider = nullptr;
};

#endif
