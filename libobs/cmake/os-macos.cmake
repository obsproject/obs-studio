find_library(COCOA Cocoa)
find_library(COREAUDIO CoreAudio)
find_library(AUDIOTOOLBOX AudioToolbox)
find_library(AUDIOUNIT AudioUnit)
find_library(APPKIT AppKit)
find_library(IOKIT IOKit)
find_library(CARBON Carbon)

mark_as_advanced(
  COCOA
  COREAUDIO
  AUDIOTOOLBOX
  AUDIOUNIT
  APPKIT
  IOKIT
  CARBON)

target_link_libraries(
  libobs
  PRIVATE ${COCOA}
          ${COREAUDIO}
          ${AUDIOTOOLBOX}
          ${AUDIOUNIT}
          ${APPKIT}
          ${IOKIT}
          ${CARBON})

target_sources(
  libobs
  PRIVATE obs-cocoa.m
          audio-monitoring/osx/coreaudio-enum-devices.c
          audio-monitoring/osx/coreaudio-monitoring-available.c
          audio-monitoring/osx/coreaudio-output.c
          audio-monitoring/osx/mac-helpers.h
          util/pipe-posix.c
          util/platform-cocoa.m
          util/platform-nix.c
          util/threading-posix.c
          util/threading-posix.h
          util/apple/cfstring-utils.h)

set_property(SOURCE util/platform-cocoa.m obs-cocoa.m PROPERTY COMPILE_FLAGS -fobjc-arc)
set_property(TARGET libobs PROPERTY FRAMEWORK TRUE)
