cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Widgets)

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(Qt6 REQUIRED Gui)
  find_package(X11 REQUIRED)
endif()

add_library(frontend-tools MODULE)
add_library(OBS::frontend-tools ALIAS frontend-tools)

if(NOT TARGET OBS::properties-view)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/properties-view" "${CMAKE_BINARY_DIR}/shared/properties-view")
endif()

if(NOT TARGET OBS::qt-plain-text-edit)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/plain-text-edit" "${CMAKE_BINARY_DIR}/shared/qt/plain-text-edit")
endif()

if(NOT TARGET OBS::qt-wrappers)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/wrappers" "${CMAKE_BINARY_DIR}/shared/qt/wrappers")
endif()

target_sources(
  frontend-tools
  PRIVATE
    $<$<PLATFORM_ID:Darwin>:auto-scene-switcher-osx.mm>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:auto-scene-switcher-nix.cpp>
    $<$<PLATFORM_ID:Windows>:auto-scene-switcher-win.cpp>
    $<$<PLATFORM_ID:Windows>:captions-handler.cpp>
    $<$<PLATFORM_ID:Windows>:captions-handler.hpp>
    $<$<PLATFORM_ID:Windows>:captions-mssapi-stream.cpp>
    $<$<PLATFORM_ID:Windows>:captions-mssapi-stream.hpp>
    $<$<PLATFORM_ID:Windows>:captions-mssapi.cpp>
    $<$<PLATFORM_ID:Windows>:captions-mssapi.hpp>
    $<$<PLATFORM_ID:Windows>:captions.cpp>
    $<$<PLATFORM_ID:Windows>:captions.hpp>
    auto-scene-switcher.cpp
    auto-scene-switcher.hpp
    frontend-tools.c
    output-timer.cpp
    output-timer.hpp
    tool-helpers.hpp
)

target_sources(
  frontend-tools
  PRIVATE forms/auto-scene-switcher.ui forms/captions.ui forms/output-timer.ui forms/scripts.ui
)

target_compile_options(
  frontend-tools
  PRIVATE $<$<PLATFORM_ID:Darwin>:-Wno-quoted-include-in-framework-header> $<$<PLATFORM_ID:Darwin>:-Wno-comma>
)

target_link_libraries(
  frontend-tools
  PRIVATE
    OBS::frontend-api
    OBS::libobs
    OBS::properties-view
    OBS::qt-plain-text-edit
    OBS::qt-wrappers
    Qt::Widgets
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa>>"
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:X11::X11>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Qt::GuiPrivate>
)

add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-scripting" "${CMAKE_BINARY_DIR}/shared/obs-scripting")

if(ENABLE_SCRIPTING AND TARGET OBS::scripting)
  target_sources(frontend-tools PRIVATE scripts.cpp scripts.hpp)
  target_link_libraries(frontend-tools PRIVATE OBS::scripting)
  target_enable_feature(frontend-tools "Scripting Support (Frontend)" ENABLE_SCRIPTING)
endif()

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in frontend-tools.rc)
  target_sources(frontend-tools PRIVATE frontend-tools.rc)

  set_property(TARGET frontend-tools APPEND PROPERTY AUTORCC_OPTIONS --format-version 1)
endif()

set_target_properties_obs(
  frontend-tools
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)
