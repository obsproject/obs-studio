cmake_minimum_required(VERSION 3.24...3.25)

add_library(mac-avcapture MODULE)
add_library(OBS::avcapture ALIAS mac-avcapture)

target_sources(mac-avcapture PRIVATE av-capture.mm left-right.hpp scope-guard.hpp)

if(NOT XCODE)
  set_source_files_properties(av-capture.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)
endif()

target_link_libraries(
  mac-avcapture
  PRIVATE OBS::libobs
          "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreMedia.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreVideo.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,CoreMediaIO.framework>")

set_target_properties_obs(
  mac-avcapture
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
