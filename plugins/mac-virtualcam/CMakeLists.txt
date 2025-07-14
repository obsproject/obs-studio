cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_VIRTUALCAM "Build OBS Virtualcam" ON)

if(NOT ENABLE_VIRTUALCAM)
  target_disable_feature(mac-virtualcam "macOS Virtual Camera support")
  return()
endif()

target_enable_feature(mac-virtualcam "macOS Virtual Camera support")

add_subdirectory(src/obs-plugin)
add_subdirectory(src/camera-extension)
