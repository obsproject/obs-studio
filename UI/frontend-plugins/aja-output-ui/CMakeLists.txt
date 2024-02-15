cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

if(NOT ENABLE_AJA)
  target_disable(aja-output-ui)
  return()
endif()

find_package(LibAJANTV2 REQUIRED)

find_package(Qt6 REQUIRED Widgets)

if(OS_LINUX
   OR OS_FREEBSD
   OR OS_OPENBSD)
  find_package(Qt6 REQUIRED Gui)

  find_package(X11 REQUIRED)
endif()

if(NOT TARGET OBS::aja-support)
  add_subdirectory("${CMAKE_SOURCE_DIR}/plugins/aja" "${CMAKE_BINARY_DIR}/plugins/aja")
endif()

add_library(aja-output-ui MODULE)
add_library(OBS::aja-output-ui ALIAS aja-output-ui)

target_sources(aja-output-ui PRIVATE # cmake-format: sortable
                                     aja-ui-main.cpp aja-ui-main.h AJAOutputUI.cpp AJAOutputUI.h)

target_sources(aja-output-ui PRIVATE forms/output.ui)

target_compile_options(
  aja-output-ui PRIVATE $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wno-deprecated-declarations>
                        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wno-quoted-include-in-framework-header>)

target_link_libraries(
  aja-output-ui
  PRIVATE OBS::libobs
          OBS::aja-support
          OBS::frontend-api
          OBS::ui-support
          Qt::Widgets
          AJA::LibAJANTV2
          $<$<PLATFORM_ID:Windows>:ws2_32.lib>
          $<$<PLATFORM_ID:Windows>:setupapi.lib>
          $<$<PLATFORM_ID:Windows>:Winmm.lib>
          $<$<PLATFORM_ID:Windows>:netapi32.lib>
          $<$<PLATFORM_ID:Windows>:Shlwapi.lib>
          "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>>"
          "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,CoreFoundation.framework>>"
          "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>>"
          $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:X11::X11>
          $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Qt::GuiPrivate>)

target_link_options(aja-output-ui PRIVATE $<$<PLATFORM_ID:Windows>:/IGNORE:4099>)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in aja-output-ui.rc)
  target_sources(aja-output-ui PRIVATE aja-output-ui.rc)

  # cmake-format: off
  set_property(TARGET aja-output-ui APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)
  # cmake-format: on
endif()

# cmake-format: off
set_target_properties_obs(
  aja-output-ui
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms)
# cmake-format: on
