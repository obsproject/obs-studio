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
  target_sources(obs-vst PRIVATE mac/EditorWidget-osx.mm mac/VSTPlugin-osx.mm)

  # cmake-format: off
  target_link_libraries(
    obs-vst
    PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
            "$<LINK_LIBRARY:FRAMEWORK,Foundation.framework>")
  # cmake-format: on
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
