cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

option(ENABLE_FFMPEG_LOGGING "Enables obs-ffmpeg logging" OFF)
option(ENABLE_NEW_MPEGTS_OUTPUT "Use native SRT/RIST mpegts output" ON)

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

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/opts-parser" "${CMAKE_BINARY_DIR}/deps/opts-parser")
endif()

add_library(obs-ffmpeg MODULE)
add_library(OBS::ffmpeg ALIAS obs-ffmpeg)

add_subdirectory(ffmpeg-mux)

target_sources(
  obs-ffmpeg
  PRIVATE obs-ffmpeg.c
          obs-ffmpeg-video-encoders.c
          obs-ffmpeg-audio-encoders.c
          obs-ffmpeg-av1.c
          obs-ffmpeg-nvenc.c
          obs-ffmpeg-output.c
          obs-ffmpeg-mux.c
          obs-ffmpeg-mux.h
          obs-ffmpeg-hls-mux.c
          obs-ffmpeg-source.c
          obs-ffmpeg-compat.h
          obs-ffmpeg-formats.h
          $<$<BOOL:${ENABLE_FFMPEG_LOGGING}>:obs-ffmpeg-logging.c>)
target_compile_definitions(obs-ffmpeg PRIVATE $<$<BOOL:${ENABLE_FFMPEG_LOGGING}>:ENABLE_FFMPEG_LOGGING>)

target_link_libraries(
  obs-ffmpeg
  PRIVATE OBS::libobs
          OBS::media-playback
          OBS::opts-parser
          FFmpeg::avcodec
          FFmpeg::avfilter
          FFmpeg::avformat
          FFmpeg::avdevice
          FFmpeg::avutil
          FFmpeg::swscale
          FFmpeg::swresample)

if(ENABLE_NEW_MPEGTS_OUTPUT)
  find_package(Librist QUIET)
  find_package(Libsrt QUIET)

  foreach(_output_lib IN ITEMS Librist Libsrt)
    if(NOT TARGET ${_output_lib}::${_output_lib})
      list(APPEND _error_messages "MPEGTS output library ${_output_lib} not found.")
    endif()
  endforeach()

  if(_error_messages)
    list(JOIN "\n" _error_string _error_string)
    message(
      FATAL_ERROR
        "${_error_string}\n Disable this error by setting ENABLE_NEW_MPEGTS_OUTPUT to OFF or providing the build system with required SRT and Rist libraries."
    )
  endif()

  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg-mpegts.c obs-ffmpeg-srt.h obs-ffmpeg-rist.h obs-ffmpeg-url.h)
  target_link_libraries(obs-ffmpeg PRIVATE Librist::Librist Libsrt::Libsrt)
  target_compile_definitions(obs-ffmpeg PRIVATE NEW_MPEGTS_OUTPUT)
endif()

if(OS_WINDOWS)
  find_package(AMF 1.4.29 REQUIRED)

  add_library(nvenc-headers INTERFACE EXCLUDE_FROM_ALL)
  add_library(OBS::nvenc-headers ALIAS nvenc-headers)

  target_sources(nvenc-headers INTERFACE external/nvEncodeAPI.h jim-nvenc-ver.h)

  target_include_directories(nvenc-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}"
                                                     "${CMAKE_CURRENT_SOURCE_DIR}/external")

  add_subdirectory(obs-amf-test)
  add_subdirectory(obs-nvenc-test)

  target_link_libraries(obs-ffmpeg PRIVATE OBS::w32-pthreads AMF::AMF ws2_32)

  configure_file(cmake/windows/obs-module.rc.in obs-ffmpeg.rc)
  target_sources(
    obs-ffmpeg
    PRIVATE texture-amf.cpp
            texture-amf-opts.hpp
            jim-nvenc.c
            jim-nvenc.h
            jim-nvenc-helpers.c
            jim-nvenc-ver.h
            obs-ffmpeg.rc)
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(Libva REQUIRED)
  find_package(Libpci REQUIRED)

  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg-vaapi.c vaapi-utils.c vaapi-utils.h)
  target_link_libraries(obs-ffmpeg PRIVATE Libva::va Libva::drm Libpci::pci)
endif()

set_target_properties_obs(obs-ffmpeg PROPERTIES FOLDER plugins/obs-ffmpeg PREFIX "")
