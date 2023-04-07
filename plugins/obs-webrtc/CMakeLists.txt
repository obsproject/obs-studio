cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

option(ENABLE_WEBRTC "Enable WebRTC Output support" ON)
if(NOT ENABLE_WEBRTC)
  message(STATUS "OBS:  DISABLED   obs-webrtc")
  return()
endif()

find_package(LibDataChannel REQUIRED)
find_package(CURL REQUIRED)

add_library(obs-webrtc MODULE)
add_library(OBS::webrtc ALIAS obs-webrtc)

target_sources(obs-webrtc PRIVATE obs-webrtc.cpp whip-output.cpp whip-output.h whip-service.cpp whip-service.h)

target_link_libraries(obs-webrtc PRIVATE OBS::libobs LibDataChannel::LibDataChannel CURL::libcurl)

set_target_properties_obs(obs-webrtc PROPERTIES FOLDER plugins PREFIX "")
