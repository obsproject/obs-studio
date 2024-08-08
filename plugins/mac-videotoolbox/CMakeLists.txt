cmake_minimum_required(VERSION 3.24...3.25)

add_library(mac-videotoolbox MODULE)
add_library(OBS::mac-videotoolbox ALIAS mac-videotoolbox)

target_sources(mac-videotoolbox PRIVATE encoder.c)

target_link_libraries(
  mac-videotoolbox
  PRIVATE
    OBS::libobs
    "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreMedia.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreVideo.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,VideoToolbox.framework>"
)

set_target_properties_obs(
  mac-videotoolbox
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES
)
