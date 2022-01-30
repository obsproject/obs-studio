# Universal build on all architectures
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

project(hal-plugin)

find_library(COREFOUNDATION CoreFoundation)
find_library(COREAUDIO CoreAudio)

mark_as_advanced(COREFOUNDATION COREAUDIO)

add_library(hal-plugin MODULE)
add_library(OBS::hal-plugin ALIAS hal-plugin)

target_sources(hal-plugin PRIVATE AudioDevice.c AudioDevice.h)

target_link_libraries(hal-plugin ${COREFOUNDATION} ${COREAUDIO})

set(MACOSX_PLUGIN_BUNDLE_NAME "OBS Desktop Audio Capture")
set(MACOSX_PLUGIN_BUNDLE_TYPE "BNDL")

set_target_properties(
  hal-plugin
  PROPERTIES BUNDLE TRUE
             BUNDLE_EXTENSION "driver"
             OUTPUT_NAME "OBSDesktopAudioCapture"
             FOLDER "plugins/mac-capture"
             MACOSX_BUNDLE_INFO_PLIST
             "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/HALPlugin-Info.plist.in")

setup_binary_target(hal-plugin)
