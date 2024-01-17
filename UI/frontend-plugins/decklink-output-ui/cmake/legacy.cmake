project(decklink-output-ui)

if(NOT ENABLE_DECKLINK)
  return()
endif()

add_library(decklink-output-ui MODULE)
add_library(OBS::decklink-output-ui ALIAS decklink-output-ui)

if(NOT TARGET OBS::properties-view)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/properties-view" "${CMAKE_BINARY_DIR}/shared/properties-view")
endif()

find_qt(COMPONENTS Widgets COMPONENTS_LINUX Gui)

set_target_properties(
  decklink-output-ui
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS "forms")

if(OS_WINDOWS)
  set_target_properties(decklink-output-ui PROPERTIES AUTORCC_OPTIONS "--format-version;1")
endif()

target_sources(decklink-output-ui PRIVATE forms/output.ui)

target_sources(decklink-output-ui PRIVATE DecklinkOutputUI.cpp DecklinkOutputUI.h decklink-ui-main.cpp
                                          decklink-ui-main.h)

target_link_libraries(decklink-output-ui PRIVATE OBS::libobs OBS::frontend-api OBS::properties-view Qt::Widgets)

target_compile_features(decklink-output-ui PRIVATE cxx_std_17)

set_target_properties(decklink-output-ui PROPERTIES FOLDER "frontend" PREFIX "")

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS Decklink Output UI")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in decklink-output-ui.rc)

  target_sources(decklink-output-ui PRIVATE decklink-output-ui.rc)

elseif(OS_MACOS)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA)

  target_link_libraries(decklink-output-ui PRIVATE ${COCOA})

elseif(OS_POSIX)
  find_package(X11 REQUIRED)
  target_link_libraries(decklink-output-ui PRIVATE X11::X11 Qt::GuiPrivate)
endif()

get_target_property(_SOURCES decklink-output-ui SOURCES)
set(_UI ${_SOURCES})
list(FILTER _UI INCLUDE REGEX ".*\\.ui?")

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}/forms"
  PREFIX "UI Files"
  FILES ${_UI})
unset(_SOURCES)
unset(_UI)

setup_plugin_target(decklink-output-ui)
