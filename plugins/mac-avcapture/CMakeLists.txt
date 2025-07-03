cmake_minimum_required(VERSION 3.28...3.30)

add_library(mac-avcapture-legacy MODULE)
add_library(OBS::avcapture-legacy ALIAS mac-avcapture-legacy)

target_sources(mac-avcapture-legacy PRIVATE legacy/av-capture.mm legacy/left-right.hpp legacy/scope-guard.hpp)
target_include_directories(mac-avcapture-legacy PRIVATE legacy)

target_link_libraries(
  mac-avcapture-legacy
  PRIVATE
    OBS::libobs
    "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreMedia.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreVideo.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreMediaIO.framework>"
)

set_target_properties_obs(
  mac-avcapture-legacy
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES
)

add_library(mac-avcapture MODULE)
add_library(OBS::avcapture ALIAS mac-avcapture)

target_sources(
  mac-avcapture
  PRIVATE
    AVCaptureDeviceFormat+OBSListable.m
    AVCaptureDeviceFormat+OBSListable.h
    plugin-main.m
    plugin-main.h
    plugin-properties.m
    plugin-properties.h
    OBSAVCapture.m
    OBSAVCapture.h
    OBSAVCapturePresetInfo.m
    OBSAVCapturePresetInfo.h
)

target_link_libraries(mac-avcapture PRIVATE OBS::libobs)

set_target_properties_obs(
  mac-avcapture
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES
             XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
             XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK YES
             XCODE_ATTRIBUTE_GCC_STRICT_ALIASING YES
)

string(TIMESTAMP CURRENT_YEAR "%Y")
set_target_xcode_properties(
  mac-avcapture
  PROPERTIES INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2023-${CURRENT_YEAR} Patrick Heyer"
)
