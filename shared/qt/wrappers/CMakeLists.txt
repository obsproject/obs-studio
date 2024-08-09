cmake_minimum_required(VERSION 3.22...3.25)

find_package(Qt6 REQUIRED Core Widgets)

add_library(qt-wrappers INTERFACE)
add_library(OBS::qt-wrappers ALIAS qt-wrappers)

target_sources(qt-wrappers INTERFACE qt-wrappers.cpp qt-wrappers.hpp)
target_include_directories(qt-wrappers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-wrappers INTERFACE Qt::Core Qt::Widgets OBS::libobs)
