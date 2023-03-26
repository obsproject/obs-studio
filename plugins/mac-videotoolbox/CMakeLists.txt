cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_library(AVFOUNDATION AVFoundation)
find_library(COCOA Cocoa)
find_library(COREFOUNDATION CoreFoundation)
find_library(COREVIDEO CoreVideo)
find_library(VIDEOTOOLBOX VideoToolbox)
find_library(COREMEDIA CoreMedia)

mark_as_advanced(AVFOUNDATION COCOA COREFOUNDATION COREVIDEO VIDEOTOOLBOX COREMEDIA)

add_library(mac-videotoolbox MODULE)
add_library(OBS::mac-videotoolbox ALIAS mac-videotoolbox)

target_sources(mac-videotoolbox PRIVATE encoder.c)

target_link_libraries(
  mac-videotoolbox
  PRIVATE OBS::libobs
          ${AVFOUNDATION}
          ${COCOA}
          ${COREFOUNDATION}
          ${COREVIDEO}
          ${VIDEOTOOLBOX}
          ${COREMEDIA})

set_target_properties_obs(
  mac-videotoolbox
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
