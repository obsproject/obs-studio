project(aja-output-ui)

if(NOT ENABLE_AJA)
  return()
endif()

find_package(LibAJANTV2 REQUIRED)

add_library(aja-output-ui MODULE)
add_library(OBS::aja-output-ui ALIAS aja-output-ui)

if(NOT TARGET OBS::properties-view)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/properties-view" "${CMAKE_BINARY_DIR}/shared/properties-view")
endif()

find_qt(COMPONENTS Widgets COMPONENTS_LINUX Gui)

set_target_properties(
  aja-output-ui
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS "forms")

if(OS_WINDOWS)
  set_target_properties(aja-output-ui PROPERTIES AUTORCC_OPTIONS "--format-version;1")
endif()

target_sources(aja-output-ui PRIVATE forms/output.ui)

target_sources(
  aja-output-ui
  PRIVATE AJAOutputUI.h
          AJAOutputUI.cpp
          aja-ui-main.cpp
          aja-ui-main.h
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-card-manager.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-card-manager.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-common.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-common.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-enums.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-presets.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-presets.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-props.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-props.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-routing.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-routing.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-ui-props.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-vpid-data.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-vpid-data.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-widget-io.cpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-widget-io.hpp)

target_link_libraries(aja-output-ui PRIVATE OBS::libobs OBS::frontend-api OBS::properties-view Qt::Widgets
                                            AJA::LibAJANTV2)

if(OS_MACOS)
  find_library(IOKIT_FRAMEWORK Iokit)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(APPKIT_FRAMEWORK AppKit)

  target_link_libraries(aja-output-ui PRIVATE ${IOKIT} ${COREFOUNDATION} ${APPKIT})
elseif(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS AJA Output UI")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in aja-output-ui.rc)
  target_sources(aja-output-ui PRIVATE aja-output-ui.rc)

  target_compile_options(aja-output-ui PRIVATE /wd4996)
  target_link_libraries(aja-output-ui PRIVATE ws2_32.lib setupapi.lib Winmm.lib netapi32.lib Shlwapi.lib)
  target_link_options(aja-output-ui PRIVATE "LINKER:/IGNORE:4099")
else()
  find_package(X11 REQUIRED)
  target_link_libraries(aja-output-ui PRIVATE X11::X11 Qt::GuiPrivate)
endif()

if(NOT MSVC)
  target_compile_options(aja-output-ui PRIVATE -Wno-error=deprecated-declarations)
endif()

set_target_properties(aja-output-ui PROPERTIES FOLDER "frontend" PREFIX "")

get_target_property(_SOURCES aja-output-ui SOURCES)
set(_UI ${_SOURCES})
list(FILTER _UI INCLUDE REGEX ".*\\.ui?")

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}/forms"
  PREFIX "UI Files"
  FILES ${_UI})
unset(_SOURCES)
unset(_UI)

setup_plugin_target(aja-output-ui)
