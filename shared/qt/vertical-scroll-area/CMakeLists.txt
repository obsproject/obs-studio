cmake_minimum_required(VERSION 3.22...3.25)

find_package(Qt6 REQUIRED Core Widgets)

add_library(qt-vertical-scroll-area INTERFACE)
add_library(OBS::qt-vertical-scroll-area ALIAS qt-vertical-scroll-area)

target_sources(qt-vertical-scroll-area INTERFACE vertical-scroll-area.cpp vertical-scroll-area.hpp)
target_include_directories(qt-vertical-scroll-area INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-vertical-scroll-area INTERFACE Qt::Core Qt::Widgets)
