cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

if(NOT ENABLE_DECKLINK)
  target_disable(decklink-output-ui)
  return()
endif()

find_qt(COMPONENTS Widgets COMPONENTS_LINUX Gui)

add_library(decklink-output-ui MODULE)
add_library(OBS::decklink-output-ui ALIAS decklink-output-ui)

target_sources(decklink-output-ui PRIVATE forms/output.ui)

target_sources(
  decklink-output-ui
  PRIVATE DecklinkOutputUI.cpp
          DecklinkOutputUI.h
          decklink-ui-main.cpp
          decklink-ui-main.h
          "${CMAKE_SOURCE_DIR}/UI/double-slider.cpp"
          "${CMAKE_SOURCE_DIR}/UI/double-slider.hpp"
          "${CMAKE_SOURCE_DIR}/UI/plain-text-edit.hpp"
          "${CMAKE_SOURCE_DIR}/UI/plain-text-edit.cpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.hpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.cpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.moc.hpp"
          "${CMAKE_SOURCE_DIR}/UI/qt-wrappers.hpp"
          "${CMAKE_SOURCE_DIR}/UI/qt-wrappers.cpp"
          "${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.cpp"
          "${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.hpp"
          "${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.cpp"
          "${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.hpp"
          "${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.hpp"
          "${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.cpp")

target_link_libraries(decklink-output-ui PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in decklink-output-ui.rc)
  target_sources(decklink-output-ui PRIVATE decklink-output-ui.rc)

  set_property(
    TARGET decklink-output-ui
    APPEND
    PROPERTY AUTORCC_OPTIONS --format-version 1)
elseif(OS_MACOS)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA)

  target_link_libraries(decklink-output-ui PRIVATE ${COCOA})
  target_compile_options(decklink-output-ui PRIVATE -Wno-quoted-include-in-framework-header)
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(X11 REQUIRED)
  target_link_libraries(decklink-output-ui PRIVATE X11::X11 Qt::GuiPrivate)
endif()

set_target_properties_obs(
  decklink-output-ui
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms)
