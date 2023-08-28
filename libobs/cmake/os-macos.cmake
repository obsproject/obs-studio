target_link_libraries(
  libobs
  PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,AudioToolbox.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,AudioUnit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,Carbon.framework>")

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

target_compile_options(libobs PUBLIC -Wno-strict-prototypes -Wno-shorten-64-to-32)

set_property(SOURCE util/platform-cocoa.m obs-cocoa.m PROPERTY COMPILE_FLAGS -fobjc-arc)
set_property(TARGET libobs PROPERTY FRAMEWORK TRUE)
