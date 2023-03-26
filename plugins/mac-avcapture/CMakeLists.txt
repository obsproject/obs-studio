cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_library(AVFOUNDATION AVFoundation)
find_library(COCOA Cocoa)
find_library(COREFOUNDATION CoreFoundation)
find_library(COREMEDIA CoreMedia)
find_library(COREVIDEO CoreVideo)
find_library(COREMEDIAIO CoreMediaIO)
mark_as_advanced(AVFOUNDATION COCOA COREFOUNDATION COREMEDIA COREMEDIAIO COREVIDEO)

add_library(mac-avcapture MODULE)
add_library(OBS::avcapture ALIAS mac-avcapture)

target_sources(mac-avcapture PRIVATE av-capture.mm left-right.hpp scope-guard.hpp)

if(NOT XCODE)
  set_source_files_properties(av-capture.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)
endif()

target_link_libraries(
  mac-avcapture
  PRIVATE OBS::libobs
          ${AVFOUNDATION}
          ${COCOA}
          ${COREFOUNDATION}
          ${COREMEDIA}
          ${COREVIDEO}
          ${COREMEDIAIO})

set_target_properties_obs(
  mac-avcapture
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
