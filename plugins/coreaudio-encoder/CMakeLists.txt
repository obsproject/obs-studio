cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

if(OS_WINDOWS)
  option(ENABLE_COREAUDIO_ENCODER "Enable building with CoreAudio encoder (Windows)" ON)
  if(NOT ENABLE_COREAUDIO_ENCODER)
    target_disable(coreaudio-encoder)
    return()
  endif()
endif()

add_library(coreaudio-encoder MODULE)
add_library(OBS::coreaudio-encoder ALIAS coreaudio-encoder)

target_sources(coreaudio-encoder PRIVATE encoder.cpp)
target_link_libraries(coreaudio-encoder PRIVATE OBS::libobs)

if(OS_WINDOWS)
  target_compile_definitions(coreaudio-encoder PRIVATE UNICODE _UNICODE)

  configure_file(cmake/windows/obs-module.rc.in coreaudio-encoder.rc)

  target_sources(coreaudio-encoder PRIVATE coreaudio-encoder.rc windows-imports.h)
elseif(OS_MACOS)
  find_library(COREFOUNDATION CoreFoundation)
  find_library(COREAUDIO CoreAudio)
  find_library(AUDIOTOOLBOX AudioToolbox)
  mark_as_advanced(AUDIOTOOLBOX COREAUDIO COREFOUNDATION)

  target_link_libraries(coreaudio-encoder PRIVATE ${COREFOUNDATION} ${COREAUDIO} ${AUDIOTOOLBOX})

endif()

set_target_properties_obs(coreaudio-encoder PROPERTIES FOLDER plugins PREFIX "")
