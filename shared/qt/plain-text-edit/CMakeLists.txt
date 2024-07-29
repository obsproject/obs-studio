cmake_minimum_required(VERSION 3.22...3.25)

find_package(Qt6 REQUIRED Core Widgets)

add_library(qt-plain-text-edit INTERFACE)
add_library(OBS::qt-plain-text-edit ALIAS qt-plain-text-edit)

target_sources(qt-plain-text-edit INTERFACE plain-text-edit.cpp plain-text-edit.hpp)
target_include_directories(qt-plain-text-edit INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-plain-text-edit INTERFACE Qt::Core Qt::Widgets)
