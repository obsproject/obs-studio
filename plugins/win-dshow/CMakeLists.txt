cmake_minimum_required(VERSION 3.24...3.25)

find_package(FFmpeg REQUIRED avcodec avutil)

add_library(win-dshow MODULE)
add_library(OBS::dshow ALIAS win-dshow)

if(NOT TARGET OBS::tiny-nv12-scale)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-tiny-nv12-scale" obs-tiny-nv12-scale)
endif()

if(NOT TARGET OBS::shared-memory-queue)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-shared-memory-queue" obs-shared-memory-queue)
endif()

if(NOT TARGET OBS::libdshowcapture)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/libdshowcapture" libdshowcapture)
endif()

if(NOT TARGET OBS::winhandle)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

target_sources(
  win-dshow
  PRIVATE dshow-plugin.cpp encode-dstr.hpp ffmpeg-decode.c ffmpeg-decode.h win-dshow-encoder.cpp win-dshow.cpp
)

configure_file(cmake/windows/obs-module.rc.in win-dshow.rc)
target_sources(win-dshow PRIVATE win-dshow.rc)

target_link_libraries(
  win-dshow
  PRIVATE
    OBS::libobs
    OBS::w32-pthreads
    OBS::libdshowcapture
    OBS::tiny-nv12-scale
    OBS::shared-memory-queue
    OBS::winhandle
    FFmpeg::avcodec
    FFmpeg::avutil
    strmiids
    winmm
)

if(TARGET OBS::virtualcam AND TARGET OBS::virtualcam-guid)
  target_sources(win-dshow PRIVATE virtualcam.c)
  target_link_libraries(win-dshow PRIVATE OBS::virtualcam-interface OBS::virtualcam-guid)
  target_compile_definitions(win-dshow PRIVATE VIRTUALCAM_AVAILABLE)

  add_dependencies(win-dshow obs-virtualcam-module)
endif()

set_target_properties_obs(win-dshow PROPERTIES FOLDER plugins/win-dshow PREFIX "")

add_subdirectory(virtualcam-module)
