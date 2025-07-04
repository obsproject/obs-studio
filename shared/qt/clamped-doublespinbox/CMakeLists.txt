cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Widgets)

add_library(qt-clamped-doublespinbox INTERFACE)
add_library(OBS::qt-clamped-doublespinbox ALIAS qt-clamped-doublespinbox)

target_sources(qt-clamped-doublespinbox INTERFACE ClampedDoubleSpinBox.cpp ClampedDoubleSpinBox.hpp)
target_include_directories(qt-clamped-doublespinbox INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(qt-clamped-doublespinbox INTERFACE Qt::Core Qt::Widgets)
