cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

if(NOT ENABLE_AJA)
  target_disable(aja-output-ui)
  return()
endif()

find_package(LibAJANTV2 REQUIRED)
find_qt(COMPONENTS Widgets COMPONENTS_LINUX Gui)

add_library(aja-output-ui MODULE)
add_library(OBS::aja-output-ui ALIAS aja-output-ui)

target_sources(
  aja-output-ui
  PRIVATE AJAOutputUI.h
          AJAOutputUI.cpp
          aja-ui-main.cpp
          aja-ui-main.h
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-card-manager.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-card-manager.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-common.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-common.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-enums.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-presets.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-presets.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-props.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-props.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-routing.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-routing.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-ui-props.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-vpid-data.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-vpid-data.hpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-widget-io.cpp"
          "${CMAKE_SOURCE_DIR}/plugins/aja/aja-widget-io.hpp"
          "${CMAKE_SOURCE_DIR}/UI/double-slider.cpp"
          "${CMAKE_SOURCE_DIR}/UI/double-slider.hpp"
          "${CMAKE_SOURCE_DIR}/UI/plain-text-edit.hpp"
          "${CMAKE_SOURCE_DIR}/UI/plain-text-edit.cpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.hpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.cpp"
          "${CMAKE_SOURCE_DIR}/UI/properties-view.moc.hpp"
          "${CMAKE_SOURCE_DIR}/UI/qt-wrappers.cpp"
          "${CMAKE_SOURCE_DIR}/UI/qt-wrappers.hpp"
          "${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.cpp"
          "${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.hpp"
          "${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.cpp"
          "${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.hpp"
          "${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.cpp"
          "${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.hpp")

target_sources(aja-output-ui PRIVATE forms/output.ui)

target_link_libraries(aja-output-ui PRIVATE OBS::libobs OBS::frontend-api Qt::Widgets AJA::LibAJANTV2)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in aja-output-ui.rc)
  target_sources(aja-output-ui PRIVATE aja-output-ui.rc)
  target_compile_options(aja-output-ui PRIVATE /wd4996)

  target_link_libraries(aja-output-ui PRIVATE ws2_32.lib setupapi.lib Winmm.lib netapi32.lib Shlwapi.lib)
  target_link_options(aja-output-ui PRIVATE /IGNORE:4099)

  set_property(
    TARGET aja-output-ui
    APPEND
    PROPERTY AUTORCC_OPTIONS --format-version 1)
elseif(OS_MACOS)
  find_library(IOKIT_FRAMEWORK Iokit)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(APPKIT_FRAMEWORK AppKit)

  target_link_libraries(aja-output-ui PRIVATE ${IOKIT} ${COREFOUNDATION} ${APPKIT})
  target_compile_options(aja-output-ui PRIVATE -Wno-error=deprecated-declarations)
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(X11 REQUIRED)
  target_link_libraries(aja-output-ui PRIVATE X11::X11 Qt::GuiPrivate)
endif()

set_target_properties_obs(
  aja-output-ui
  PROPERTIES FOLDER frontend
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS forms)
