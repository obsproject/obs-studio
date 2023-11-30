cmake_minimum_required(VERSION 3.22...3.25)

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
  PRIVATE # cmake-format: sortable
          $<$<BOOL:${ENABLE_FFMPEG_LOGGING}>:obs-ffmpeg-logging.c>
          obs-ffmpeg-audio-encoders.c
          obs-ffmpeg-av1.c
          obs-ffmpeg-compat.h
          obs-ffmpeg-formats.h
          obs-ffmpeg-hls-mux.c
          obs-ffmpeg-mux.c
          obs-ffmpeg-mux.h
          obs-ffmpeg-nvenc.c
          obs-ffmpeg-output.c
          obs-ffmpeg-output.h
          obs-ffmpeg-source.c
          obs-ffmpeg-video-encoders.c
          obs-ffmpeg.c)

target_compile_options(obs-ffmpeg PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>)
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
  find_package(FFnvcodec 12 REQUIRED)

  add_library(obs-nvenc-version INTERFACE)
  add_library(OBS::obs-nvenc-version ALIAS obs-nvenc-version)
  target_sources(obs-nvenc-version INTERFACE obs-nvenc-ver.h)
  target_include_directories(obs-nvenc-version INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

  add_subdirectory(obs-amf-test)
  add_subdirectory(obs-nvenc-test)

  target_link_libraries(obs-ffmpeg PRIVATE OBS::obs-nvenc-version OBS::w32-pthreads AMF::AMF FFnvcodec::FFnvcodec
                                           ws2_32)

  configure_file(cmake/windows/obs-module.rc.in obs-ffmpeg.rc)
  target_sources(obs-ffmpeg PRIVATE obs-nvenc-helpers.c obs-nvenc.c obs-nvenc.h obs-ffmpeg.rc texture-amf-opts.hpp
                                    texture-amf.cpp)
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(Libva REQUIRED)
  find_package(Libpci REQUIRED)

  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg-vaapi.c vaapi-utils.c vaapi-utils.h)
  target_link_libraries(obs-ffmpeg PRIVATE Libva::va Libva::drm Libpci::pci)
endif()

# cmake-format: off
set_target_properties_obs(obs-ffmpeg PROPERTIES FOLDER plugins/obs-ffmpeg PREFIX "")
# cmake-format: on
