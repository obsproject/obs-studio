cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_LIBFDK "Enable FDK AAC support" OFF)
if(NOT ENABLE_LIBFDK)
  target_disable(obs-libfdk)
  return()
endif()

find_package(Libfdk REQUIRED)

add_library(obs-libfdk MODULE)
add_library(OBS::libfdk ALIAS obs-libfdk)

target_sources(obs-libfdk PRIVATE obs-libfdk.c)

target_link_libraries(obs-libfdk PRIVATE OBS::libobs Libfdk::Libfdk)

set_target_properties_obs(obs-libfdk PROPERTIES FOLDER plugins PREFIX "")
