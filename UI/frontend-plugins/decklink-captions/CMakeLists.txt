cmake_minimum_required(VERSION 3.28...3.30)

if(NOT ENABLE_DECKLINK)
  target_disable(decklink-captions)
  return()
endif()

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(X11 REQUIRED)
  mark_as_advanced(X11)
endif()

find_package(Qt6 REQUIRED Widgets)

add_library(decklink-captions MODULE)
add_library(OBS::decklink-captions ALIAS decklink-captions)

target_sources(decklink-captions PRIVATE decklink-captions.cpp decklink-captions.h forms/captions.ui)

target_compile_options(
  decklink-captions
  PRIVATE $<$<PLATFORM_ID:Darwin>:-Wno-quoted-include-in-framework-header> $<$<PLATFORM_ID:Darwin>:-Wno-comma>
)

target_link_libraries(
  decklink-captions
  PRIVATE
    OBS::frontend-api
    OBS::libobs
    Qt::Widgets
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>"
    $<$<PLATFORM_ID:Linux,FreeBSD>:X11::X11>
)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in decklink-captions.rc)
  target_sources(decklink-captions PRIVATE decklink-captions.rc)

  set_property(TARGET decklink-captions APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)
endif()

set_target_properties_obs(
  decklink-captions
  PROPERTIES FOLDER plugins/decklink
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms
)
