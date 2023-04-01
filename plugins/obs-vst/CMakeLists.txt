cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

option(ENABLE_VST "Enable building OBS with VST plugin" ON)

if(NOT ENABLE_VST)
  target_disable(obs-vst)
  return()
endif()

add_library(obs-vst MODULE)
add_library(OBS::vst ALIAS obs-vst)

find_qt(COMPONENTS Widgets)

target_sources(
  obs-vst
  PRIVATE obs-vst.cpp
          VSTPlugin.cpp
          EditorWidget.cpp
          headers/vst-plugin-callbacks.hpp
          headers/EditorWidget.h
          headers/VSTPlugin.h
          vst_header/aeffectx.h)

target_include_directories(obs-vst PRIVATE vst_header)
target_link_libraries(obs-vst PRIVATE OBS::libobs Qt::Widgets)

if(OS_MACOS)
  find_library(FOUNDATION Foundation)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA FOUNDATION)

  target_sources(obs-vst PRIVATE mac/VSTPlugin-osx.mm mac/EditorWidget-osx.mm)
  target_link_libraries(obs-vst PRIVATE ${COCOA} ${FOUNDATION})

elseif(OS_WINDOWS)
  target_sources(obs-vst PRIVATE win/VSTPlugin-win.cpp win/EditorWidget-win.cpp)

  set_property(
    TARGET obs-vst
    APPEND
    PROPERTY AUTORCC_OPTIONS --format-version 1)

elseif(OS_POSIX)
  target_sources(obs-vst PRIVATE linux/VSTPlugin-linux.cpp linux/EditorWidget-linux.cpp)
endif()

set_target_properties_obs(
  obs-vst
  PROPERTIES FOLDER plugins
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)
