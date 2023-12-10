cmake_minimum_required(VERSION 3.24...3.25)

# cmake-format: off
add_library(ipc-util STATIC EXCLUDE_FROM_ALL)
# cmake-format: on
add_library(OBS::ipc-util ALIAS ipc-util)

target_sources(ipc-util PRIVATE ipc-util/pipe.h)
target_include_directories(ipc-util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources(ipc-util PRIVATE ipc-util/pipe-windows.c ipc-util/pipe-windows.h)
target_compile_options(ipc-util PRIVATE $<IF:$<CONFIG:Debug>,/MTd,/MT> /Zl)

set_target_properties(ipc-util PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE ON)
