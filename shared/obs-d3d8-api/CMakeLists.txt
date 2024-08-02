cmake_minimum_required(VERSION 3.22...3.25)

add_library(d3d8-api INTERFACE)
add_library(OBS::d3d8-api ALIAS d3d8-api)
target_sources(d3d8-api INTERFACE d3d8.h d3d8caps.h d3d8types.h)
target_include_directories(d3d8-api INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
