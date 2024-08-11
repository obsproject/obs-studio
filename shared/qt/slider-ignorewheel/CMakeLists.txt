cmake_minimum_required(VERSION 3.22...3.25)

find_package(Qt6 REQUIRED Core Widgets)

add_library(qt-slider-ignorewheel INTERFACE)
add_library(OBS::qt-slider-ignorewheel ALIAS qt-slider-ignorewheel)

target_sources(qt-slider-ignorewheel INTERFACE slider-ignorewheel.cpp slider-ignorewheel.hpp)
target_include_directories(qt-slider-ignorewheel INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-slider-ignorewheel INTERFACE Qt::Core Qt::Widgets OBS::libobs)
