cmake_minimum_required(VERSION 3.22...3.25)

if(NOT TARGET OBS::tiny-nv12-scale)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-tiny-nv12-scale" obs-tiny-nv12-scale)
endif()

add_library(obs-shared-memory-queue INTERFACE)
add_library(OBS::shared-memory-queue ALIAS obs-shared-memory-queue)
target_sources(obs-shared-memory-queue INTERFACE shared-memory-queue.c shared-memory-queue.h)
target_include_directories(obs-shared-memory-queue INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(obs-shared-memory-queue INTERFACE OBS::tiny-nv12-scale)
