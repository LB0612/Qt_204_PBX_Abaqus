#include "ResultViewerWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QPixmap>
#include <QFrame>

namespace {

const char *kCardStyle =
    "QFrame {"
    "  background-color: #ffffff;"
    "  border: 1px solid #e8e8e8;"
    "  border-radius: 8px;"
    "}";

} // namespace

ResultViewerWidget::ResultViewerWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(
        QStringLiteral(
            "ResultViewerWidget { background-color: #ffffff; }"
        )
    );

    playTimer = new QTimer(this);
    playTimer->setTimerType(Qt::CoarseTimer);
    connect(playTimer, &QTimer::timeout, this, [this]() {
        if (totalFrames <= 0) {
            stopPlayback();
            return;
        }

        if (currentFrame >= totalFrames - 1) {
            stopPlayback();
            return;
        }

        loadFrame(currentFrame + 1);
    });

    buildUi();
}

void ResultViewerWidget::buildUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    titleLabel = new QLabel(QStringLiteral("仿真结果"), this);
    titleLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 20px;"
            "font-weight: bold;"
            "color: #333333;"
        )
    );
    layout->addWidget(titleLabel);

    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->setSpacing(12);

    cureButton = new QPushButton(QStringLiteral("固化度云图"), this);
    temperatureButton = new QPushButton(QStringLiteral("温度云图"), this);
    stressButton = new QPushButton(QStringLiteral("应力云图"), this);

    for (QPushButton *button : {
             cureButton,
             temperatureButton,
             stressButton}) {
        button->setMinimumHeight(36);
        button->setStyleSheet(
            QStringLiteral(
                "QPushButton {"
                "  font-family: 'Microsoft YaHei';"
                "  font-size: 14px;"
                "  padding: 6px 16px;"
                "  border: 1px solid #d9d9d9;"
                "  border-radius: 6px;"
                "  background: #ffffff;"
                "}"
                "QPushButton:hover { background: #f5f5f5; }"
            )
        );
        typeLayout->addWidget(button);
    }
    typeLayout->addStretch();
    layout->addLayout(typeLayout);

    connect(cureButton, &QPushButton::clicked, this, [this]() {
        switchResultType(ResultType::Cure);
    });
    connect(temperatureButton, &QPushButton::clicked, this, [this]() {
        switchResultType(ResultType::Temperature);
    });
    connect(stressButton, &QPushButton::clicked, this, [this]() {
        switchResultType(ResultType::Stress);
    });

    QFrame *imageCard = new QFrame(this);
    imageCard->setStyleSheet(QString::fromUtf8(kCardStyle));
    QVBoxLayout *imageLayout = new QVBoxLayout(imageCard);
    imageLayout->setContentsMargins(12, 12, 12, 12);

    imageLabel = new QLabel(imageCard);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(360);
    imageLabel->setStyleSheet(
        QStringLiteral(
            "background-color: #fafafa;"
            "border: 1px solid #eeeeee;"
            "border-radius: 6px;"
        )
    );
    imageLayout->addWidget(imageLabel, 1);

    messageLabel = new QLabel(imageCard);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 15px;"
            "color: #666666;"
            "padding: 24px;"
        )
    );
    messageLabel->hide();
    imageLayout->addWidget(messageLabel);

    layout->addWidget(imageCard, 1);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(12);

    prevButton = new QPushButton(QStringLiteral("◀ 上一帧"), this);
    playButton = new QPushButton(QStringLiteral("▶ 播放"), this);
    nextButton = new QPushButton(QStringLiteral("下一帧 ▶"), this);

    for (QPushButton *button : {
             prevButton,
             playButton,
             nextButton}) {
        button->setMinimumHeight(34);
    }

    controlLayout->addWidget(prevButton);
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(nextButton);
    controlLayout->addStretch();
    layout->addLayout(controlLayout);

    connect(prevButton, &QPushButton::clicked, this, [this]() {
        stepFrame(-1);
    });
    connect(playButton, &QPushButton::clicked, this, [this]() {
        togglePlayback();
    });
    connect(nextButton, &QPushButton::clicked, this, [this]() {
        stepFrame(1);
    });

    frameSlider = new QSlider(Qt::Horizontal, this);
    frameSlider->setMinimum(0);
    frameSlider->setMaximum(0);
    connect(frameSlider, &QSlider::valueChanged, this, [this](int value) {
        if (value != currentFrame) {
            stopPlayback();
            loadFrame(value);
        }
    });
    layout->addWidget(frameSlider);

    frameInfoLabel = new QLabel(this);
    simulationTimeLabel = new QLabel(this);
    resultNameLabel = new QLabel(this);
    metaLabel = new QLabel(this);

    for (QLabel *label : {
             frameInfoLabel,
             simulationTimeLabel,
             resultNameLabel,
             metaLabel}) {
        label->setStyleSheet(
            QStringLiteral(
                "font-family: 'Microsoft YaHei';"
                "font-size: 14px;"
                "color: #555555;"
            )
        );
    }

    layout->addWidget(frameInfoLabel);
    layout->addWidget(simulationTimeLabel);
    layout->addWidget(resultNameLabel);
    layout->addWidget(metaLabel);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    openDirButton = new QPushButton(QStringLiteral("打开结果目录"), this);
    reportButton = new QPushButton(QStringLiteral("生成PDF报告"), this);
    bottomLayout->addWidget(openDirButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(reportButton);
    layout->addLayout(bottomLayout);

    connect(openDirButton, &QPushButton::clicked, this, [this]() {
        emit openResultsDirectoryRequested();
    });
    connect(reportButton, &QPushButton::clicked, this, [this]() {
        emit generateReportRequested();
    });

    setControlsEnabled(false);
}

void ResultViewerWidget::setProjectPath(const QString &path)
{
    if (projectPath != path) {
        stopPlayback();
        projectPath = path;
        imageLabel->clear();
        currentFrame = 0;
        totalFrames = 0;
    }
}

void ResultViewerWidget::refreshResults()
{
    stopPlayback();

    if (projectPath.isEmpty()) {
        validation = ResultValidationResult();
        showMessage(QStringLiteral("当前未打开工程。"));
        setControlsEnabled(false);
        return;
    }

    validation = SimulationResultService::validate(projectPath);
    if (!validation.isValid()) {
        showMessage(validation.message);
        setControlsEnabled(false);
        return;
    }

    hideMessage();
    setControlsEnabled(true);

    totalFrames = validation.manifest.odbFrames;
    fps = validation.manifest.videoFps > 0
        ? validation.manifest.videoFps
        : 5;

    frameSlider->setMaximum(qMax(0, totalFrames - 1));
    switchResultType(ResultType::Cure);
}

void ResultViewerWidget::stopPlayback()
{
    if (playTimer->isActive()) {
        playTimer->stop();
    }
    playButton->setText(QStringLiteral("▶ 播放"));
}

void ResultViewerWidget::setControlsEnabled(bool enabled)
{
    const QList<QWidget *> widgets = {
        cureButton,
        temperatureButton,
        stressButton,
        prevButton,
        playButton,
        nextButton,
        frameSlider,
        openDirButton,
        reportButton,
    };

    for (QWidget *widget : widgets) {
        widget->setEnabled(enabled);
    }
}

void ResultViewerWidget::showMessage(const QString &text)
{
    messageLabel->setText(text);
    messageLabel->show();
    imageLabel->hide();
}

void ResultViewerWidget::hideMessage()
{
    messageLabel->hide();
    imageLabel->show();
}

void ResultViewerWidget::switchResultType(ResultType type)
{
    stopPlayback();
    currentType = type;
    currentFrame = 0;
    loadFrame(0);
}

void ResultViewerWidget::loadFrame(int frameIndex)
{
    if (!validation.isValid() || projectPath.isEmpty()) {
        return;
    }

    if (totalFrames <= 0) {
        return;
    }

    frameIndex = qBound(0, frameIndex, totalFrames - 1);
    currentFrame = frameIndex;

    const QString pngPath =
        SimulationResultService::framePngPath(
            projectPath,
            currentType,
            frameIndex
        );

    QPixmap pixmap(pngPath);
    if (pixmap.isNull()) {
        imageLabel->setText(
            QStringLiteral("无法加载当前帧：%1").arg(pngPath)
        );
    } else {
        imageLabel->setPixmap(
            pixmap.scaled(
                imageLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            )
        );
    }

    if (frameSlider->value() != currentFrame) {
        frameSlider->setValue(currentFrame);
    }

    updateFrameInfo();
}

void ResultViewerWidget::updateFrameInfo()
{
    const ResultTypeDescriptor desc =
        SimulationResultService::descriptorFor(currentType);

    frameInfoLabel->setText(
        QStringLiteral("帧：%1 / %2")
            .arg(currentFrame + 1)
            .arg(totalFrames)
    );

    if (validation.manifest.frameTimes.size() == totalFrames) {
        simulationTimeLabel->setText(
            QStringLiteral("仿真时间：%1 s")
                .arg(
                    validation.manifest.frameTimes.at(currentFrame),
                    0,
                    'f',
                    2
                )
        );
    } else {
        simulationTimeLabel->setText(
            QStringLiteral("仿真时间：-")
        );
    }

    resultNameLabel->setText(
        QStringLiteral("当前结果：%1").arg(desc.displayName)
    );
    metaLabel->setText(
        QStringLiteral("总帧数：%1    播放帧率：%2 FPS")
            .arg(totalFrames)
            .arg(fps)
    );
}

void ResultViewerWidget::togglePlayback()
{
    if (!validation.isValid() || totalFrames <= 0) {
        return;
    }

    if (playTimer->isActive()) {
        stopPlayback();
        return;
    }

    if (currentFrame >= totalFrames - 1) {
        loadFrame(0);
    }

    const int intervalMs = qMax(1, 1000 / fps);
    playTimer->start(intervalMs);
    playButton->setText(QStringLiteral("暂停"));
}

void ResultViewerWidget::stepFrame(int delta)
{
    if (!validation.isValid() || totalFrames <= 0) {
        return;
    }

    stopPlayback();
    loadFrame(currentFrame + delta);
}
