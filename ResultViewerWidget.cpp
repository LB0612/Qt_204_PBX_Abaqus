#include "ResultViewerWidget.h"

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const char *kCardStyle =
    "QFrame {"
    "  background-color: #ffffff;"
    "  border: 1px solid #e8e8e8;"
    "  border-radius: 8px;"
    "}";

const char *kTypeButtonStyle =
    "QPushButton {"
    "  font-family: 'Microsoft YaHei';"
    "  font-size: 14px;"
    "  padding: 6px 16px;"
    "  border: 1px solid #d9d9d9;"
    "  border-radius: 6px;"
    "  background: #ffffff;"
    "  color: #333333;"
    "}"
    "QPushButton:hover { background: #f5f5f5; }"
    "QPushButton:checked {"
    "  background: #1890ff;"
    "  color: #ffffff;"
    "  border-color: #1890ff;"
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
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(16);

    titleLabel = new QLabel(QStringLiteral("仿真结果"), this);
    titleLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 20px;"
            "font-weight: bold;"
            "color: #333333;"
        )
    );
    rootLayout->addWidget(titleLabel);

    QHBoxLayout *centerLayout = new QHBoxLayout();
    centerLayout->addStretch();

    contentContainer = new QWidget(this);
    contentContainer->setMaximumWidth(1100);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);

    emptyStatePanel = new QWidget(contentContainer);
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStatePanel);
    emptyLayout->setContentsMargins(0, 40, 0, 40);
    emptyLayout->addStretch();

    QFrame *emptyStateCard = new QFrame(emptyStatePanel);
    emptyStateCard->setStyleSheet(QString::fromUtf8(kCardStyle));
    QVBoxLayout *emptyCardLayout = new QVBoxLayout(emptyStateCard);
    emptyCardLayout->setContentsMargins(48, 40, 48, 40);
    emptyCardLayout->setSpacing(16);

    emptyStateTitleLabel = new QLabel(emptyStateCard);
    emptyStateTitleLabel->setAlignment(Qt::AlignCenter);
    emptyStateTitleLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 18px;"
            "font-weight: bold;"
            "color: #333333;"
            "border: none;"
            "background: transparent;"
        )
    );

    emptyStateMessageLabel = new QLabel(emptyStateCard);
    emptyStateMessageLabel->setAlignment(Qt::AlignCenter);
    emptyStateMessageLabel->setWordWrap(true);
    emptyStateMessageLabel->setStyleSheet(
        QStringLiteral(
            "font-family: 'Microsoft YaHei';"
            "font-size: 14px;"
            "color: #666666;"
            "border: none;"
            "background: transparent;"
        )
    );

    continueButton = new QPushButton(emptyStateCard);
    continueButton->setMinimumHeight(36);
    continueButton->setMinimumWidth(160);
    continueButton->setStyleSheet(
        QStringLiteral(
            "QPushButton {"
            "  font-family: 'Microsoft YaHei';"
            "  font-size: 14px;"
            "  padding: 6px 20px;"
            "  border: 1px solid #1890ff;"
            "  border-radius: 6px;"
            "  background: #1890ff;"
            "  color: #ffffff;"
            "}"
            "QPushButton:hover { background: #40a9ff; border-color: #40a9ff; }"
        )
    );

    emptyCardLayout->addWidget(emptyStateTitleLabel);
    emptyCardLayout->addWidget(emptyStateMessageLabel);

    QHBoxLayout *continueLayout = new QHBoxLayout();
    continueLayout->addStretch();
    continueLayout->addWidget(continueButton);
    continueLayout->addStretch();
    emptyCardLayout->addLayout(continueLayout);

    emptyLayout->addWidget(emptyStateCard);
    emptyLayout->addStretch();
    contentLayout->addWidget(emptyStatePanel);

    playerPanel = new QWidget(contentContainer);
    QVBoxLayout *playerLayout = new QVBoxLayout(playerPanel);
    playerLayout->setContentsMargins(0, 0, 0, 0);
    playerLayout->setSpacing(16);

    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->setSpacing(12);

    cureButton = new QPushButton(QStringLiteral("固化度云图"), playerPanel);
    temperatureButton = new QPushButton(QStringLiteral("温度云图"), playerPanel);
    stressButton = new QPushButton(QStringLiteral("应力云图"), playerPanel);

    typeButtonGroup = new QButtonGroup(this);
    typeButtonGroup->setExclusive(true);

    for (QPushButton *button : {
             cureButton,
             temperatureButton,
             stressButton}) {
        button->setCheckable(true);
        button->setMinimumHeight(36);
        button->setStyleSheet(QString::fromUtf8(kTypeButtonStyle));
        typeButtonGroup->addButton(button);
        typeLayout->addWidget(button);
    }
    typeLayout->addStretch();
    playerLayout->addLayout(typeLayout);

    connect(cureButton, &QPushButton::clicked, this, [this]() {
        switchResultType(ResultType::Cure);
        togglePlayback();
    });
    connect(temperatureButton, &QPushButton::clicked, this, [this]() {
        switchResultType(ResultType::Temperature);
        togglePlayback();
    });
    connect(stressButton, &QPushButton::clicked, this, [this]() {
        switchResultType(ResultType::Stress);
        togglePlayback();
    });

    QFrame *imageCard = new QFrame(playerPanel);
    imageCard->setStyleSheet(QString::fromUtf8(kCardStyle));
    QVBoxLayout *imageLayout = new QVBoxLayout(imageCard);
    imageLayout->setContentsMargins(12, 12, 12, 12);

    imageLabel = new QLabel(imageCard);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(420);
    imageLabel->setStyleSheet(
        QStringLiteral(
            "background-color: #fafafa;"
            "border: 1px solid #eeeeee;"
            "border-radius: 6px;"
        )
    );
    imageLayout->addWidget(imageLabel, 1);
    playerLayout->addWidget(imageCard, 1);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(12);

    prevButton = new QPushButton(QStringLiteral("◀ 上一帧"), playerPanel);
    playButton = new QPushButton(QStringLiteral("▶ 播放"), playerPanel);
    nextButton = new QPushButton(QStringLiteral("下一帧 ▶"), playerPanel);

    for (QPushButton *button : {
             prevButton,
             playButton,
             nextButton}) {
        button->setMinimumHeight(34);
    }

    controlLayout->addStretch();
    controlLayout->addWidget(prevButton);
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(nextButton);
    controlLayout->addStretch();
    playerLayout->addLayout(controlLayout);

    connect(prevButton, &QPushButton::clicked, this, [this]() {
        stepFrame(-1);
    });
    connect(playButton, &QPushButton::clicked, this, [this]() {
        togglePlayback();
    });
    connect(nextButton, &QPushButton::clicked, this, [this]() {
        stepFrame(1);
    });

    frameSlider = new QSlider(Qt::Horizontal, playerPanel);
    frameSlider->setMinimum(0);
    frameSlider->setMaximum(0);
    connect(frameSlider, &QSlider::valueChanged, this, [this](int value) {
        if (value != currentFrame) {
            stopPlayback();
            loadFrame(value);
        }
    });
    playerLayout->addWidget(frameSlider);

    infoPrimaryLabel = new QLabel(playerPanel);
    infoSecondaryLabel = new QLabel(playerPanel);
    for (QLabel *label : {infoPrimaryLabel, infoSecondaryLabel}) {
        label->setStyleSheet(
            QStringLiteral(
                "font-family: 'Microsoft YaHei';"
                "font-size: 14px;"
                "color: #555555;"
            )
        );
    }
    playerLayout->addWidget(infoPrimaryLabel);
    playerLayout->addWidget(infoSecondaryLabel);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    openDirButton = new QPushButton(QStringLiteral("打开结果目录"), playerPanel);
    reportButton = new QPushButton(QStringLiteral("生成PDF报告"), playerPanel);
    bottomLayout->addWidget(openDirButton);
    bottomLayout->addStretch();
    bottomLayout->addWidget(reportButton);
    playerLayout->addLayout(bottomLayout);

    connect(openDirButton, &QPushButton::clicked, this, [this]() {
        emit openResultsDirectoryRequested();
    });
    connect(reportButton, &QPushButton::clicked, this, [this]() {
        emit generateReportRequested();
    });
    connect(continueButton, &QPushButton::clicked, this, [this]() {
        emit continueSimulationRequested();
    });

    contentLayout->addWidget(playerPanel);
    centerLayout->addWidget(contentContainer, 1);
    centerLayout->addStretch();
    rootLayout->addLayout(centerLayout, 1);

    setResultUiVisible(false);
    showEmptyState(ResultValidationResult());
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
        validation.state = ResultValidationState::ProjectMissing;
        validation.message = QStringLiteral("当前未打开工程。");
        showEmptyState(validation);
        setResultUiVisible(false);
        return;
    }

    validation = SimulationResultService::validate(projectPath);
    if (!validation.isValid()) {
        showEmptyState(validation);
        setResultUiVisible(false);
        return;
    }

    setResultUiVisible(true);

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

void ResultViewerWidget::setResultUiVisible(bool visible)
{
    playerPanel->setVisible(visible);
    emptyStatePanel->setVisible(!visible);
}

void ResultViewerWidget::showEmptyState(const ResultValidationResult &result)
{
    emptyStateTitleLabel->setText(emptyStateTitle(result.state));
    emptyStateMessageLabel->setText(
        result.message.isEmpty()
            ? QStringLiteral("请先完成 Abaqus 仿真及后处理。")
            : result.message
    );
    continueButton->setText(continueButtonText(result.state));
}

QString ResultViewerWidget::emptyStateTitle(
    ResultValidationState state) const
{
    switch (state) {
    case ResultValidationState::PostIncomplete:
        return QStringLiteral("后处理尚未完成");
    case ResultValidationState::PostShaMismatch:
        return QStringLiteral("仿真结果与参数不一致");
    case ResultValidationState::OutputsInvalid:
        return QStringLiteral("仿真结果文件不完整");
    case ResultValidationState::ManifestInvalid:
        return QStringLiteral("后处理清单无效");
    case ResultValidationState::NoResults:
        return QStringLiteral("尚无完整仿真结果");
    case ResultValidationState::ProjectMissing:
        return QStringLiteral("当前未打开工程");
    case ResultValidationState::Valid:
        break;
    }

    return QStringLiteral("尚无完整仿真结果");
}

QString ResultViewerWidget::continueButtonText(
    ResultValidationState state) const
{
    switch (state) {
    case ResultValidationState::PostIncomplete:
    case ResultValidationState::OutputsInvalid:
        return QStringLiteral("继续后处理");
    case ResultValidationState::PostShaMismatch:
        return QStringLiteral("重新进行仿真");
    case ResultValidationState::NoResults:
    case ResultValidationState::ProjectMissing:
    case ResultValidationState::ManifestInvalid:
        return QStringLiteral("开始仿真");
    case ResultValidationState::Valid:
        break;
    }

    return QStringLiteral("开始仿真");
}

void ResultViewerWidget::switchResultType(ResultType type)
{
    stopPlayback();
    currentType = type;
    currentFrame = 0;

    switch (type) {
    case ResultType::Cure:
        cureButton->setChecked(true);
        break;
    case ResultType::Temperature:
        temperatureButton->setChecked(true);
        break;
    case ResultType::Stress:
        stressButton->setChecked(true);
        break;
    }

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

    infoPrimaryLabel->setText(
        QStringLiteral("当前结果：%1      Frame：%2 / %3")
            .arg(desc.displayName)
            .arg(currentFrame + 1)
            .arg(totalFrames)
    );

    QString simulationTimeText = QStringLiteral("仿真时间：-");
    if (validation.manifest.frameTimes.size() == totalFrames) {
        simulationTimeText =
            QStringLiteral("仿真时间：%1 s")
                .arg(
                    validation.manifest.frameTimes.at(currentFrame),
                    0,
                    'f',
                    2
                );
    }

    infoSecondaryLabel->setText(
        QStringLiteral("%1                  %2 FPS")
            .arg(simulationTimeText)
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
