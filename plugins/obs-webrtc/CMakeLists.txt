cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_WEBRTC "Enable WebRTC Output support" ON)
if(NOT ENABLE_WEBRTC)
  target_disable(obs-webrtc)
  return()
endif()

find_package(LibDataChannel 0.20 REQUIRED)
find_package(CURL REQUIRED)
find_package(
  FFmpeg REQUIRED
  COMPONENTS avcodec
             avfilter
             avdevice
             avutil
             swscale
             avformat
             swresample)

if(NOT TARGET OBS::media-playback)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/media-playback" "${CMAKE_BINARY_DIR}/deps/media-playback")
endif()

add_library(obs-webrtc MODULE)
add_library(OBS::webrtc ALIAS obs-webrtc)

target_sources(
  obs-webrtc
  PRIVATE # cmake-format: sortable
          obs-webrtc.cpp
          webrtc-utils.h
          whep-source.cpp
          whep-source.h
          whip-output.cpp
          whip-output.h
          whip-service.cpp
          whip-service.h)

target_link_libraries(
  obs-webrtc
  PRIVATE OBS::libobs
          OBS::media-playback
          LibDataChannel::LibDataChannel
          CURL::libcurl
          FFmpeg::avcodec
          FFmpeg::avfilter
          FFmpeg::avformat
          FFmpeg::avdevice
          FFmpeg::avutil
          FFmpeg::swscale
          FFmpeg::swresample)

# cmake-format: off
set_target_properties_obs(obs-webrtc PROPERTIES FOLDER plugins PREFIX "")
# cmake-format: on
