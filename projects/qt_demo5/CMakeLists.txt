cmake_minimum_required(VERSION 3.16)

project("qt_demo")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT qt_demo)

add_subdirectory("qt_demo")