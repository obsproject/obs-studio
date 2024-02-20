cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_VST "Enable building OBS with VST plugin" ON)

if(NOT ENABLE_VST)
  target_disable(obs-vst)
  return()
endif()

add_library(obs-vst MODULE)
add_library(OBS::vst ALIAS obs-vst)

find_package(Qt6 REQUIRED Widgets)

target_sources(
  obs-vst
  PRIVATE # cmake-format: sortable
          $<$<PLATFORM_ID:Darwin>:mac/EditorWidget-osx.mm>
          $<$<PLATFORM_ID:Darwin>:mac/VSTPlugin-osx.mm>
          $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:linux/EditorWidget-linux.cpp>
          $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:linux/VSTPlugin-linux.cpp>
          $<$<PLATFORM_ID:Windows>:win/EditorWidget-win.cpp>
          $<$<PLATFORM_ID:Windows>:win/VSTPlugin-win.cpp>
          EditorWidget.cpp
          headers/EditorWidget.h
          headers/vst-plugin-callbacks.hpp
          headers/VSTPlugin.h
          obs-vst.cpp
          vst_header/aeffectx.h
          VSTPlugin.cpp)

target_include_directories(obs-vst PRIVATE vst_header)

# cmake-format: off
target_link_libraries(
  obs-vst PRIVATE OBS::libobs
                  Qt::Widgets
                  "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>"
                  "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Foundation.framework>>")
# cmake-format: on

if(OS_WINDOWS)
  set_property(
    TARGET obs-vst
    APPEND
    PROPERTY AUTORCC_OPTIONS --format-version 1)
endif()

# cmake-format: off
set_target_properties_obs(
  obs-vst
  PROPERTIES FOLDER plugins
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)
# cmake-format: on
