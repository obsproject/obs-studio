cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_VIRTUALCAM "Build OBS Virtualcam" ON)

if(NOT ENABLE_VIRTUALCAM)
  target_disable_feature(mac-virtualcam "macOS Virtual Camera support")
  return()
endif()

target_enable_feature(mac-virtualcam "macOS Virtual Camera support")

add_library(obs-mach-protocol INTERFACE)
add_library(OBS::mach-protocol ALIAS obs-mach-protocol)

target_sources(obs-mach-protocol INTERFACE src/common/MachProtocol.h src/common/data/placeholder.png)
target_include_directories(obs-mach-protocol INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/common")

add_subdirectory(src/obs-plugin)
add_subdirectory(src/dal-plugin)
