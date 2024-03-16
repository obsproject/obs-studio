cmake_minimum_required(VERSION 3.24...3.25)

add_library(ipc-util INTERFACE)
add_library(OBS::ipc-util ALIAS ipc-util)

target_sources(
  ipc-util INTERFACE "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/ipc-util/pipe-windows.c>" ipc-util/pipe.h
                     "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/ipc-util/pipe-windows.h>")

target_include_directories(ipc-util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(ipc-util PROPERTIES FOLDER deps)
