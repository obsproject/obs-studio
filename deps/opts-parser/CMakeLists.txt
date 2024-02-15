cmake_minimum_required(VERSION 3.22...3.25)

add_library(opts-parser OBJECT)
add_library(OBS::opts-parser ALIAS opts-parser)

target_sources(
  opts-parser
  PRIVATE opts-parser.c
  PUBLIC opts-parser.h)

target_include_directories(opts-parser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(opts-parser PUBLIC OBS::libobs)

set_target_properties(opts-parser PROPERTIES FOLDER deps)
