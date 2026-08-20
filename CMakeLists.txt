cmake_minimum_required(VERSION 3.16)

project(QT_PBX_204_ABAQUS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    BaseParamWidget.cpp
    BaseParamWidget.h
    ProjectManager.cpp
    ProjectManager.h
    ProjectInfoWidget.cpp
    ProjectInfoWidget.h
    SimulationManager.cpp
    SimulationManager.h
    NewProjectDialog.cpp
    NewProjectDialog.h
    OpenProjectDialog.cpp
    OpenProjectDialog.h
    SettingsDialog.cpp
    SettingsDialog.h
    StructureConfig.h
    StructureConfigManager.cpp
    StructureConfigManager.h
    StructureParamWidget.cpp
    StructureParamWidget.h
    AbaqusFileGenerator.cpp
    AbaqusFileGenerator.h
    Picture.qrc
    SimulationTemplates.qrc
)

qt_standard_project_setup()

qt_add_executable(QT_PBX_204_ABAQUS
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(QT_PBX_204_ABAQUS PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

qt_finalize_executable(QT_PBX_204_ABAQUS)
