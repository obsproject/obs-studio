cmake_minimum_required(VERSION 3.22...3.25)

add_library(hook-config INTERFACE)
add_library(OBS::hook-config ALIAS hook-config)
target_sources(hook-config INTERFACE graphics-hook-ver.h graphics-hook-info.h hook-helpers.h)
target_include_directories(hook-config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
