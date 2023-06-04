cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

option(ENABLE_AJA "Build OBS with aja support" ON)

if(NOT ENABLE_AJA)
  set_property(GLOBAL APPEND PROPERTY OBS_MODULES_DISABLED aja)
  return()
endif()

find_package(LibAJANTV2 REQUIRED)

add_library(aja MODULE)
add_library(OBS::aja ALIAS aja)

target_sources(
  aja
  PRIVATE main.cpp
          aja-card-manager.cpp
          aja-common.cpp
          aja-common.hpp
          aja-output.cpp
          aja-enums.hpp
          aja-output.hpp
          aja-presets.cpp
          aja-presets.hpp
          aja-props.cpp
          aja-props.hpp
          aja-routing.cpp
          aja-routing.hpp
          aja-source.cpp
          aja-source.hpp
          aja-vpid-data.cpp
          aja-vpid-data.hpp
          aja-widget-io.cpp
          aja-widget-io.hpp
          aja-card-manager.hpp
          aja-ui-props.hpp
          audio-repack.c
          audio-repack.h
          audio-repack.hpp)

target_link_libraries(aja PRIVATE OBS::libobs AJA::LibAJANTV2)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in win-aja.rc)
  target_sources(aja PRIVATE win-aja.rc)

  target_compile_options(aja PRIVATE /wd4996)

  target_link_libraries(aja PRIVATE ws2_32.lib setupapi.lib Winmm.lib netapi32.lib Shlwapi.lib)
  target_link_options(aja PRIVATE /IGNORE:4099)

elseif(OS_MACOS)
  find_library(IOKIT IOKit)
  find_library(COREFOUNDATION CoreFoundation)
  find_library(APPKIT AppKit)

  target_link_libraries(aja PRIVATE ${IOKIT} ${COREFOUNDATION} ${APPKIT})
  target_compile_options(aja PRIVATE -Wno-error=deprecated-declarations)
elseif(OS_LINUX OR OS_FREEBSD)
  target_compile_options(aja PRIVATE -Wno-error=deprecated-declarations)
endif()

set_target_properties_obs(aja PROPERTIES FOLDER plugins PREFIX "")
