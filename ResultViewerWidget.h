#ifndef RESULTVIEWERWIDGET_H
#define RESULTVIEWERWIDGET_H

#include "SimulationResultService.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QButtonGroup;
class QFrame;

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
    void continueSimulationRequested();

private:
    void buildUi();
    void setResultUiVisible(bool visible);
    void showEmptyState(const ResultValidationResult &result);
    void switchResultType(ResultType type);
    void loadFrame(int frameIndex);
    void updateFrameInfo();
    void togglePlayback();
    void stepFrame(int delta);
    QString emptyStateTitle(ResultValidationState state) const;
    QString continueButtonText(ResultValidationState state) const;

    QString projectPath;
    ResultValidationResult validation;

    ResultType currentType = ResultType::Cure;
    int currentFrame = 0;
    int totalFrames = 0;
    int fps = 5;

    QTimer *playTimer = nullptr;
    QButtonGroup *typeButtonGroup = nullptr;

    QLabel *titleLabel = nullptr;

    QWidget *contentContainer = nullptr;
    QWidget *playerPanel = nullptr;
    QWidget *emptyStatePanel = nullptr;

    QLabel *emptyStateTitleLabel = nullptr;
    QLabel *emptyStateMessageLabel = nullptr;
    QPushButton *continueButton = nullptr;

    QLabel *imageLabel = nullptr;
    QLabel *infoPrimaryLabel = nullptr;
    QLabel *infoSecondaryLabel = nullptr;

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
