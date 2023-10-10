project(obs-webrtc)

option(ENABLE_WEBRTC "Enable WebRTC Output support" ON)
if(NOT ENABLE_WEBRTC)
  obs_status(DISABLED "obs-webrtc")
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
  PRIVATE obs-webrtc.cpp
          whip-output.cpp
          whip-output.h
          whip-service.cpp
          whip-service.h
          whep-source.cpp
          whep-source.h
          webrtc-utils.h)

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

set_target_properties(obs-webrtc PROPERTIES FOLDER "plugins")

setup_plugin_target(obs-webrtc)
