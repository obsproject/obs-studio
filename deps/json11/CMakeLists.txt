cmake_minimum_required(VERSION 3.16...3.25)

add_library(json11 INTERFACE)
add_library(OBS::json11 ALIAS json11)

target_include_directories(json11 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(json11 INTERFACE json11.cpp json11.hpp)
