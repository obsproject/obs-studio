target_link_libraries(
  libobs
  PRIVATE
    "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AudioToolbox.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AudioUnit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Carbon.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>"
)

target_sources(
  libobs
  PRIVATE
    audio-monitoring/osx/coreaudio-enum-devices.c
    audio-monitoring/osx/coreaudio-monitoring-available.c
    audio-monitoring/osx/coreaudio-output.c
    audio-monitoring/osx/mac-helpers.h
    obs-cocoa.m
    util/apple/cfstring-utils.h
    util/pipe-posix.c
    util/platform-cocoa.m
    util/platform-nix.c
    util/threading-posix.c
    util/threading-posix.h
)

target_compile_options(libobs PUBLIC "$<$<NOT:$<COMPILE_LANGUAGE:Swift>>:-Wno-strict-prototypes;-Wno-shorten-64-to-32>")

set_property(SOURCE obs-cocoa.m util/platform-cocoa.m PROPERTY COMPILE_OPTIONS -fobjc-arc)
set_property(TARGET libobs PROPERTY FRAMEWORK TRUE)
