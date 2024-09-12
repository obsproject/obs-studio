cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_OSS "Enable building with OSS audio support" ON)

if(NOT ENABLE_OSS)
  target_disable(oss-audio)
  return()
endif()

find_package(OSS REQUIRED)

add_library(oss-audio MODULE)
add_library(OBS::oss-audio ALIAS oss-audio)

target_sources(oss-audio PRIVATE oss-audio.c oss-input.c)

target_link_libraries(oss-audio PRIVATE OBS::libobs OSS::OSS)

set_target_properties_obs(oss-audio PROPERTIES FOLDER plugins PREFIX "")
