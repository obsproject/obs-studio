cmake_minimum_required(VERSION 3.22...3.25)

add_library(uthash INTERFACE)
add_library(OBS::uthash ALIAS uthash)

target_sources(uthash INTERFACE uthash/uthash.h)

target_include_directories(uthash INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
