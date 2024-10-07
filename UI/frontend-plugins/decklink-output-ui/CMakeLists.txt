cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

if(NOT ENABLE_DECKLINK)
  target_disable(decklink-output-ui)
  return()
endif()

find_package(Qt6 REQUIRED Widgets)

if(OS_LINUX
   OR OS_FREEBSD
   OR OS_OPENBSD)
  find_package(Qt6 REQUIRED Gui)

  find_package(X11 REQUIRED)
endif()

add_library(decklink-output-ui MODULE)
add_library(OBS::decklink-output-ui ALIAS decklink-output-ui)

target_sources(decklink-output-ui PRIVATE forms/output.ui)

target_sources(decklink-output-ui PRIVATE DecklinkOutputUI.cpp DecklinkOutputUI.h decklink-ui-main.cpp
                                          decklink-ui-main.h)

target_compile_options(decklink-output-ui PRIVATE $<$<PLATFORM_ID:Darwin>:-Wno-quoted-include-in-framework-header>)

target_link_libraries(
  decklink-output-ui
  PRIVATE OBS::libobs
          OBS::frontend-api
          OBS::ui-support
          Qt::Widgets
          "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>"
          $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:X11::X11>
          $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Qt::GuiPrivate>)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in decklink-output-ui.rc)
  target_sources(decklink-output-ui PRIVATE decklink-output-ui.rc)

  # cmake-format: off
  set_property(TARGET decklink-output-ui APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)
  # cmake-format: on
endif()

set_target_properties_obs(
  decklink-output-ui
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms)
