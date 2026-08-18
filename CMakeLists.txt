cmake_minimum_required(VERSION 3.16)

project(Qt_Simulate_Project VERSION 1.0 LANGUAGES CXX)

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
    Picture.qrc
)

qt_standard_project_setup()

qt_add_executable(Qt_Simulate_Project
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(Qt_Simulate_Project PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

qt_finalize_executable(Qt_Simulate_Project)
