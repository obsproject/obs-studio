cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_LIBFDK "Enable FDK AAC support" ON)
if(NOT ENABLE_LIBFDK)
  target_disable(obs-libfdk)
  return()
endif()

add_library(obs-libfdk MODULE)
add_library(OBS::libfdk ALIAS obs-libfdk)

target_sources(obs-libfdk PRIVATE obs-libfdk.c)

target_link_libraries(obs-libfdk PRIVATE OBS::libobs)

set_target_properties_obs(obs-libfdk PROPERTIES FOLDER plugins PREFIX "")
