cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_WEBRTC "Enable WebRTC Output support" ON)
if(NOT ENABLE_WEBRTC)
  target_disable(obs-webrtc)
  return()
endif()

find_package(LibDataChannel 0.20 REQUIRED)
find_package(CURL REQUIRED)

add_library(obs-webrtc MODULE)
add_library(OBS::webrtc ALIAS obs-webrtc)

target_sources(
  obs-webrtc PRIVATE # cmake-format: sortable
                     obs-webrtc.cpp whip-output.cpp whip-output.h whip-service.cpp whip-service.h whip-utils.h)

target_link_libraries(obs-webrtc PRIVATE OBS::libobs LibDataChannel::LibDataChannel CURL::libcurl)

# cmake-format: off
set_target_properties_obs(obs-webrtc PROPERTIES FOLDER plugins PREFIX "")
# cmake-format: on
