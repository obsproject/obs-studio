cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_AMF "Build AMD Radeon Hardware Encoder Plugin" ON)

if(NOT ENABLE_AMF)
  target_disable_feature(obs-amf "AMD Radeon Hardware Encoder")
  target_disable(obs-amf)
  return()
endif()

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/opts-parser" "${CMAKE_BINARY_DIR}/shared/opts-parser")
endif()

find_package(AMF 1.4.29 REQUIRED)

add_library(obs-amf MODULE)
add_library(OBS::amf ALIAS obs-amf)

add_subdirectory(obs-amf-test)

target_sources(obs-amf PRIVATE obs-amf.c texture-amf-opts.hpp texture-amf.cpp)

target_link_libraries(obs-amf PRIVATE OBS::libobs OBS::opts-parser AMF::AMF)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-amf.rc)
  target_sources(obs-amf PRIVATE obs-amf.rc)
endif()

set_target_properties_obs(obs-amf PROPERTIES FOLDER plugins/obs-amf PREFIX "")
