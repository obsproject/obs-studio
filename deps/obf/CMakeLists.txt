cmake_minimum_required(VERSION 3.16...3.25)

add_library(obf INTERFACE)
add_library(OBS::obf ALIAS obf)

target_sources(obf INTERFACE obf.c obf.h)
target_include_directories(obf INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
