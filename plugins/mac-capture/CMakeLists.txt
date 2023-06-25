cmake_minimum_required(VERSION 3.24...3.25)

add_library(mac-capture MODULE)
add_library(OBS::capture ALIAS mac-capture)

target_sources(
  mac-capture
  PRIVATE # cmake-format: sortable
          audio-device-enum.c
          audio-device-enum.h
          CGDisplayStream.h
          mac-audio.c
          mac-display-capture.m
		  mac-sck-common.h
		  mac-sck-common.m
          mac-sck-video-capture.m
          mac-window-capture.m
          plugin-main.c
          window-utils.h
          window-utils.m)

# cmake-format: off
target_link_libraries(
  mac-capture
  PRIVATE OBS::libobs
          "$<LINK_LIBRARY:FRAMEWORK,AudioUnit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreMedia.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreVideo.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,IOSurface.framework>"
          "$<LINK_LIBRARY:WEAK_FRAMEWORK,ScreenCaptureKit.framework>")

set_target_properties_obs(
  mac-capture
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
# cmake-format: on

if(CMAKE_VERSION VERSION_LESS_EQUAL 3.25.0)
  set_property(TARGET mac-capture PROPERTY XCODE_LINK_BUILD_PHASE_MODE BUILT_ONLY)
endif()
