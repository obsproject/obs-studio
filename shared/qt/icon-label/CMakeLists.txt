cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets)

add_library(qt-icon-label INTERFACE)
add_library(OBS::qt-icon-label ALIAS qt-icon-label)

target_sources(qt-icon-label INTERFACE icon-label.cpp icon-label.hpp)
target_include_directories(qt-icon-label INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-icon-label INTERFACE Qt::Core Qt::Widgets)
