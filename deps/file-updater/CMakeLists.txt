cmake_minimum_required(VERSION 3.16...3.25)

find_package(CURL REQUIRED)

add_library(file-updater INTERFACE EXCLUDE_FROM_ALL)
add_library(OBS::file-updater ALIAS file-updater)

target_sources(file-updater INTERFACE file-updater/file-updater.c file-updater/file-updater.h)

target_link_libraries(file-updater INTERFACE CURL::libcurl $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

target_include_directories(file-updater INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
