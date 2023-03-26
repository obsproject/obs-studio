cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(COREFOUNDATION CoreFoundation)
find_library(IOSURF IOSurface)
find_library(COCOA Cocoa)
find_library(COREVIDEO CoreVideo)
find_library(COREMEDIA CoreMedia)
find_library(SCREENCAPTUREKIT ScreenCaptureKit)

add_library(mac-capture MODULE)
add_library(OBS::capture ALIAS mac-capture)

target_sources(
  mac-capture
  PRIVATE plugin-main.c
          audio-device-enum.c
          audio-device-enum.h
          mac-audio.c
          mac-display-capture.m
          mac-screen-capture.m
          mac-window-capture.m
          window-utils.m
          window-utils.h)

target_link_libraries(mac-capture PRIVATE OBS::libobs ${COREAUDIO} ${AUDIOUNIT} ${COREFOUNDATION} ${IOSURF} ${COCOA})

if(SCREENCAPTUREKIT)
  target_link_libraries(mac-capture PRIVATE OBS::libobs ${COREVIDEO} ${COREMEDIA}
                                            "$<LINK_LIBRARY:WEAK_FRAMEWORK,${SCREENCAPTUREKIT}>")
endif()

set_target_properties_obs(
  mac-capture
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)

if(CMAKE_VERSION VERSION_LESS_EQUAL 3.25.0)
  set_property(TARGET mac-capture PROPERTY XCODE_LINK_BUILD_PHASE_MODE BUILT_ONLY)
endif()
