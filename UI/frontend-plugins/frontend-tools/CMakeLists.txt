cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_qt(COMPONENTS Widgets COMPONENTS_LINUX Gui)

add_library(frontend-tools MODULE)
add_library(OBS::frontend-tools ALIAS frontend-tools)

target_sources(
  frontend-tools
  PRIVATE frontend-tools.c
          auto-scene-switcher.hpp
          auto-scene-switcher.cpp
          output-timer.hpp
          tool-helpers.hpp
          output-timer.cpp
          "${CMAKE_SOURCE_DIR}/UI/double-slider.cpp"
          "${CMAKE_SOURCE_DIR}/UI/double-slider.hpp"
          "${CMAKE_SOURCE_DIR}/UI/horizontal-scroll-area.cpp"
          "${CMAKE_SOURCE_DIR}/UI/horizontal-scroll-area.hpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.cpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.hpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.moc.hpp"
          "${CMAKE_SOURCE_DIR}/UI/qt-wrappers.cpp"
          "${CMAKE_SOURCE_DIR}/UI/qt-wrappers.hpp"
          "${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.cpp"
          "${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.hpp"
          "${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.cpp"
          "${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.hpp"
          "${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.hpp"
          "${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.cpp"
          "${CMAKE_SOURCE_DIR}/UI/plain-text-edit.cpp"
          "${CMAKE_SOURCE_DIR}/UI/plain-text-edit.hpp")

target_sources(frontend-tools PRIVATE forms/auto-scene-switcher.ui forms/captions.ui forms/output-timer.ui
                                      forms/scripts.ui)

target_link_libraries(frontend-tools PRIVATE OBS::frontend-api OBS::libobs Qt::Widgets
                                             $<$<PLATFORM_ID:Linux,FreeBSD>:Qt::GuiPrivate>)

add_subdirectory("${CMAKE_SOURCE_DIR}/deps/obs-scripting" "${CMAKE_BINARY_DIR}/deps/obs-scripting")

if(ENABLE_SCRIPTING AND TARGET OBS::scripting)
  target_sources(frontend-tools PRIVATE scripts.cpp scripts.hpp)
  target_link_libraries(frontend-tools PRIVATE OBS::scripting)
  target_enable_feature(frontend-tools "Scripting Support (Frontend)" ENABLE_SCRIPTING)
endif()

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in frontend-tools.rc)
  target_sources(
    frontend-tools
    PRIVATE auto-scene-switcher-win.cpp
            frontend-tools.rc
            captions.cpp
            captions.hpp
            captions-handler.cpp
            captions-handler.hpp
            captions-mssapi.cpp
            captions-mssapi.hpp
            captions-mssapi-stream.cpp
            captions-mssapi-stream.hpp)

  set_property(
    TARGET frontend-tools
    APPEND
    PROPERTY AUTORCC_OPTIONS --format-version 1)
elseif(OS_MACOS)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA)
  target_link_libraries(frontend-tools PRIVATE ${COCOA})

  target_sources(frontend-tools PRIVATE auto-scene-switcher-osx.mm)
  target_compile_options(frontend-tools PRIVATE -Wno-quoted-include-in-framework-header -Wno-comma)
  set_target_properties(frontend-tools PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
  if(NOT XCODE)
    set_source_files_properties(auto-scene-switcher-osx.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)
  endif()
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(X11 REQUIRED)

  target_link_libraries(frontend-tools PRIVATE X11::X11)
  target_sources(frontend-tools PRIVATE auto-scene-switcher-nix.cpp)
endif()

set_target_properties_obs(
  frontend-tools
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms)
