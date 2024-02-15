cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_package(Libx264 REQUIRED)

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/opts-parser" "${CMAKE_BINARY_DIR}/deps/opts-parser")
endif()

add_library(obs-x264 MODULE)
add_library(OBS::x264 ALIAS obs-x264)

target_sources(obs-x264 PRIVATE obs-x264.c obs-x264-plugin-main.c)
target_link_libraries(obs-x264 PRIVATE OBS::opts-parser Libx264::Libx264)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-x264.rc)
  target_sources(obs-x264 PRIVATE obs-x264.rc)
endif()

# cmake-format: off
set_target_properties_obs(obs-x264 PROPERTIES FOLDER plugins/obs-x264 PREFIX "")
# cmake-format: on

include(cmake/x264-test.cmake)
