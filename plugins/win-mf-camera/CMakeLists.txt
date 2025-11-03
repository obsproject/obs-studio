# cmake-format: off
cmake_minimum_required(VERSION 3.28...3.30)

add_library(win-mf-camera MODULE)
add_library(OBS::win-mf-camera ALIAS win-mf-camera)

target_sources(
  win-mf-camera
  PRIVATE message-queue.hpp mf-camera-plugin.cpp mf-camera.cpp mf-camera.hpp)

configure_file(cmake/windows/obs-module.rc.in win-mf-camera.rc)
target_sources(win-mf-camera PRIVATE win-mf-camera.rc)

target_link_libraries(
  win-mf-camera
  PRIVATE OBS::libobs
          Avrt
          Mfplat
          Mf
          mfuuid
          Mfreadwrite
          Strmiids)
# cmake-format: on

set_target_properties_obs(win-mf-camera PROPERTIES FOLDER plugins PREFIX "")
