cmake_minimum_required(VERSION 3.16)
project(qt_demo LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
)
qt_standard_project_setup()

if(TARGET Qt5::Core AND NOT TARGET Qt::Core)
    add_library(Qt::Core ALIAS Qt5::Core)
endif()
if(TARGET Qt5::Gui AND NOT TARGET Qt::Gui)
    add_library(Qt::Gui ALIAS Qt5::Gui)
endif()
if(TARGET Qt5::Widgets AND NOT TARGET Qt::Widgets)
    add_library(Qt::Widgets ALIAS Qt5::Widgets)
endif()

set(PROJECT_SOURCES
    main.cpp
    qt_demo.h
    qt_demo.cpp
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

if(WIN32)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

