cmake_minimum_required(VERSION 3.22...3.25)

add_library(obs-tiny-nv12-scale INTERFACE)
add_library(OBS::tiny-nv12-scale ALIAS obs-tiny-nv12-scale)
target_sources(obs-tiny-nv12-scale INTERFACE tiny-nv12-scale.c tiny-nv12-scale.h)
target_include_directories(obs-tiny-nv12-scale INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
