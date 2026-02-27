cmake_minimum_required(VERSION 3.28...3.30)

add_library(mac-au MODULE)
add_library(OBS::au ALIAS mac-au)

target_sources(
  mac-au
  PRIVATE mac-au.cpp mac-au.h mac-au-scan.mm mac-au-scan.h mac-au-plugin.mm mac-au-plugin.h plugin-main.cpp
)

target_link_libraries(
  mac-au
  PRIVATE
    OBS::libobs
    "$<LINK_LIBRARY:FRAMEWORK,AudioUnit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AudioToolbox.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreAudioKit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>"
)

set_source_files_properties(mac-au-plugin.mm mac-au-scan.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

set_target_properties_obs(
  mac-au
  PROPERTIES FOLDER plugins
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES
)

if(CMAKE_VERSION VERSION_LESS_EQUAL 3.25.0)
  set_property(TARGET mac-au PROPERTY XCODE_LINK_BUILD_PHASE_MODE BUILT_ONLY)
endif()
