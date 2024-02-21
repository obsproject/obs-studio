project(obs-mpegts)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat)

add_library(obs-mpegts MODULE)
add_library(OBS::mpegts ALIAS obs-mpegts)

find_package(Librist REQUIRED)
find_package(Libsrt REQUIRED)
find_qt(COMPONENTS Core Widgets Charts)

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

target_sources(obs-mpegts PRIVATE obs-mpegts.c obs-mpegts-common.h obs-mpegts-rist.h obs-mpegts-srt.h
                                  obs-mpegts-srt-stats.cpp obs-mpegts-srt-stats.hpp)

target_link_libraries(
  obs-mpegts
  PRIVATE OBS::libobs
          OBS::frontend-api
          FFmpeg::avcodec
          FFmpeg::avformat
          FFmpeg::avutil
          Librist::Librist
          Libsrt::Libsrt
          Qt::Core
          Qt::Widgets
          Qt::Charts)

set_target_properties(obs-mpegts PROPERTIES FOLDER "plugins/obs-mpegts" PREFIX "")
set_target_properties(
  obs-mpegts
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

if(OS_WINDOWS)
  target_link_libraries(obs-mpegts PRIVATE ws2_32.lib)
  if(MSVC)
    target_link_libraries(obs-mpegts PRIVATE OBS::w32-pthreads)
  endif()

  set(MODULE_DESCRIPTION "OBS MPEG-TS muxer module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in obs-mpegts.rc)
endif()

setup_plugin_target(obs-mpegts)
