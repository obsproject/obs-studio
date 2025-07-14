cmake_minimum_required(VERSION 3.28...3.30)

add_library(mac-virtualcam MODULE)
add_library(OBS::virtualcam ALIAS mac-virtualcam)

target_sources(
  mac-virtualcam
  PRIVATE
    CoreMediaIO+Extensions.swift
    VirtualCam.swift
    libobs+Extensions.swift
    mac-virtualcam-Bridging-header.h
    plugin-main.swift
    virtualcam-output-api.swift
)

target_link_libraries(mac-virtualcam PRIVATE OBS::libobs OBS::frontend-api)

set_target_properties_obs(
  mac-virtualcam
  PROPERTIES
    FOLDER plugins
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
)

set_target_xcode_properties(
  mac-virtualcam
  PROPERTIES
    CLANG_ENABLE_MODULES YES
    CLANG_ENABLE_OBJC_ARC YES
    CLANG_MODULES_AUTOLINK YES
    CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
    DEFINES_MODULE YES
    GCC_STRICT_ALIASING YES
    GCC_WARN_SHADOW YES
    SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/mac-virtualcam-Bridging-Header.h"
    SWIFT_VERSION 6.0
)

if(CMAKE_COMPILE_WARNING_AS_ERROR)
  set_target_xcode_properties(mac-virtualcam PROPERTIES SWIFT_TREAT_WARNINGS_AS_ERRORS YES)
endif()
