cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_SYPHON "Enable Syphon sharing support" ON)
if(NOT ENABLE_SYPHON)
  target_disable(mac-syphon)
  target_disable_feature(mac-syphon "Syphon sharing support")
  return()
else()
  target_enable_feature(mac-syphon "Syphon sharing support")
endif()

find_library(SYPHON Syphon)
mark_as_advanced(SYPHON)

add_library(mac-syphon MODULE)
add_library(OBS::syphon ALIAS mac-syphon)

target_sources(mac-syphon PRIVATE syphon.m plugin-main.c SyphonOBSClient.h SyphonOBSClient.m)

target_compile_options(mac-syphon PRIVATE -fobjc-arc)
target_link_libraries(
  mac-syphon
  PRIVATE OBS::libobs "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>" "$<LINK_LIBRARY:FRAMEWORK,IOSurface.framework>"
          "$<LINK_LIBRARY:FRAMEWORK,ScriptingBridge.framework>" "$<LINK_LIBRARY:FRAMEWORK,${SYPHON}>")

set_target_properties_obs(
  mac-syphon
  PROPERTIES FOLDER "plugins"
             PREFIX ""
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
