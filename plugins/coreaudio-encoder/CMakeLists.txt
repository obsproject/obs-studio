cmake_minimum_required(VERSION 3.24...3.25)

if(OS_WINDOWS)
  option(ENABLE_COREAUDIO_ENCODER "Enable building with CoreAudio encoder (Windows)" ON)
  if(NOT ENABLE_COREAUDIO_ENCODER)
    target_disable(coreaudio-encoder)
    return()
  endif()
endif()

add_library(coreaudio-encoder MODULE)
add_library(OBS::coreaudio-encoder ALIAS coreaudio-encoder)

target_sources(coreaudio-encoder PRIVATE encoder.cpp $<$<PLATFORM_ID:Windows>:windows-imports.h>)

target_link_libraries(
  coreaudio-encoder
  PRIVATE
    OBS::libobs
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,AudioToolbox.framework>>"
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>>"
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>>"
)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in coreaudio-encoder.rc)
  target_sources(coreaudio-encoder PRIVATE coreaudio-encoder.rc)
endif()

set_target_properties_obs(coreaudio-encoder PROPERTIES FOLDER plugins PREFIX "")
