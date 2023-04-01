project(decklink-captions)

if(NOT ENABLE_DECKLINK)
  return()
endif()

add_library(decklink-captions MODULE)
add_library(OBS::decklink-captions ALIAS decklink-captions)

find_qt(COMPONENTS Widgets)

target_link_libraries(decklink-captions PRIVATE Qt::Widgets)

set_target_properties(
  decklink-captions
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS "forms")

if(_QT_VERSION EQUAL 6 AND OS_WINDOWS)
  set_target_properties(decklink-captions PROPERTIES AUTORCC_OPTIONS "--format-version;1")
endif()

target_compile_features(decklink-captions PRIVATE cxx_std_17)

target_sources(decklink-captions PRIVATE forms/captions.ui)

target_sources(decklink-captions PRIVATE decklink-captions.cpp decklink-captions.h)

target_link_libraries(decklink-captions PRIVATE OBS::frontend-api OBS::libobs)

if(OS_MACOS)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA)
  target_link_libraries(decklink-captions PRIVATE ${COCOA})

elseif(OS_POSIX)
  find_package(X11 REQUIRED)
  mark_as_advanced(X11)
  target_link_libraries(decklink-captions PRIVATE X11::X11)
elseif(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS DeckLink Captions module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in decklink-captions.rc)

  target_sources(decklink-captions PRIVATE decklink-captions.rc)
endif()

set_target_properties(decklink-captions PROPERTIES FOLDER "plugins/decklink" PREFIX "")

get_target_property(_SOURCES decklink-captions SOURCES)
set(_UI ${_SOURCES})
list(FILTER _UI INCLUDE REGEX ".*\\.ui?")

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}/forms"
  PREFIX "UI Files"
  FILES ${_UI})
unset(_SOURCES)
unset(_UI)

setup_plugin_target(decklink-captions)
