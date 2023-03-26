cmake_minimum_required(VERSION 3.16...3.25)

add_library(opts-parser INTERFACE EXCLUDE_FROM_ALL)
add_library(OBS::opts-parser ALIAS opts-parser)

target_sources(opts-parser INTERFACE opts-parser.c opts-parser.h)
target_include_directories(opts-parser INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(opts-parser INTERFACE OBS::libobs)
