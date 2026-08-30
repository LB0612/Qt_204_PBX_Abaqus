QT += core gui widgets printsupport

CONFIG += c++17

TARGET = QT_PBX_204_ABAQUS
TEMPLATE = app
VERSION = 1.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    BaseParamWidget.cpp \
    ProjectManager.cpp \
    ProjectInfoWidget.cpp \
    ProjectParameterService.cpp \
    SimulationManager.cpp \
    ProjectInputHash.cpp \
    NewProjectDialog.cpp \
    OpenProjectDialog.cpp \
    SettingsDialog.cpp \
    StructureConfigManager.cpp \
    StructureParamWidget.cpp \
    ExplosiveConfigManager.cpp \
    ExplosiveParamWidget.cpp \
    MoldConfigManager.cpp \
    MoldParamWidget.cpp \
    BoundaryConfigManager.cpp \
    BoundaryParamWidget.cpp \
    SimulationConfigManager.cpp \
    SimulationParamWidget.cpp \
    ParameterCheckWidget.cpp \
    SimulationMonitorWidget.cpp \
    SimulationPrepareWidget.cpp \
    ResultViewerWidget.cpp \
    SimulationArtifactStateService.cpp \
    SimulationResultService.cpp \
    SimulationReportGenerator.cpp \
    SimulationReportPdfWriter.cpp \
    SimulationReportDocxWriter.cpp \
    AbaqusFileGenerator.cpp

HEADERS += \
    AppInfo.h \
    ProjectPaths.h \
    mainwindow.h \
    BaseParamWidget.h \
    ProjectManager.h \
    ProjectInfoWidget.h \
    ProjectParameterService.h \
    SimulationManager.h \
    ProjectInputHash.h \
    NewProjectDialog.h \
    OpenProjectDialog.h \
    SettingsDialog.h \
    SimulationArtifactStateService.h \
    StructureConfig.h \
    StructureConfigManager.h \
    StructureParamWidget.h \
    ExplosiveConfig.h \
    ExplosiveConfigManager.h \
    ExplosiveParamWidget.h \
    MoldConfig.h \
    MoldConfigManager.h \
    MoldParamWidget.h \
    BoundaryConfig.h \
    BoundaryConfigManager.h \
    BoundaryParamWidget.h \
    SimulationConfig.h \
    SimulationConfigManager.h \
    SimulationParamWidget.h \
    ParameterCheckWidget.h \
    SimulationMonitorWidget.h \
    SimulationPrepareWidget.h \
    ResultViewerWidget.h \
    SimulationResultService.h \
    SimulationReportModel.h \
    SimulationReportStyle.h \
    SimulationReportGenerator.h \
    SimulationReportPdfWriter.h \
    SimulationReportDocxWriter.h \
    AbaqusFileGenerator.h

RESOURCES += \
    Picture.qrc \
    SimulationTemplates.qrc
