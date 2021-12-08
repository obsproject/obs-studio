project(aja-output-ui)

if(NOT ENABLE_AJA)
  return()
endif()

find_package(LibAJANTV2 REQUIRED)

add_library(aja-output-ui MODULE)
add_library(OBS::aja-output-ui ALIAS aja-output-ui)

if(BUILD_QT EQUAL 5)
  find_package(
    Qt5
    COMPONENTS Widgets
    REQUIRED)

  target_link_libraries(aja-output-ui PRIVATE Qt5::Widgets)

  set_target_properties(
    aja-output-ui
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON
               AUTOUIC_SEARCH_PATHS "forms")

  if(OS_POSIX AND NOT OS_MACOS)
    find_package(Qt5X11Extras)
    target_link_libraries(aja-output-ui PRIVATE Qt5::X11Extras)
  endif()
elseif(BUILD_QT EQUAL 6)
  message(FATAL_ERROR "OBS:    -        Building with Qt6 not supported yet.")
endif()

target_sources(
  aja-output-ui
  PRIVATE AJAOutputUI.h
          AJAOutputUI.cpp
          aja-ui-main.cpp
          aja-ui-main.h
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-ui-props.hpp
          ${CMAKE_SOURCE_DIR}/plugins/aja/aja-enums.hpp
          ${CMAKE_SOURCE_DIR}/UI/combobox-ignorewheel.cpp
          ${CMAKE_SOURCE_DIR}/UI/combobox-ignorewheel.hpp
          ${CMAKE_SOURCE_DIR}/UI/double-slider.cpp
          ${CMAKE_SOURCE_DIR}/UI/double-slider.hpp
          ${CMAKE_SOURCE_DIR}/UI/properties-view.hpp
          ${CMAKE_SOURCE_DIR}/UI/properties-view.cpp
          ${CMAKE_SOURCE_DIR}/UI/properties-view.moc.hpp
          ${CMAKE_SOURCE_DIR}/UI/qt-wrappers.cpp
          ${CMAKE_SOURCE_DIR}/UI/qt-wrappers.hpp
          ${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.cpp
          ${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.hpp
          ${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.cpp
          ${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.hpp
          ${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.cpp
          ${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.hpp)

target_link_libraries(aja-output-ui PRIVATE OBS::libobs OBS::frontend-api
                                            AJA::LibAJANTV2)

if(TARGET AJA::LibAJANTV2_Debug)
  target_link_libraries(aja-output-ui PRIVATE debug AJA::LibAJANTV2_Debug)
endif()

if(OS_MACOS)
  find_library(IOKIT_FRAMEWORK Iokit)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(APPKIT_FRAMEWORK AppKit)

  target_link_libraries(aja-output-ui PRIVATE ${IOKIT} ${COREFOUNDATION}
                                              ${APPKIT})
elseif(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS AJA Output UI")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 aja-output-ui.rc)
  target_sources(aja-output-ui PRIVATE aja-output-ui.rc)

  target_link_libraries(aja-output-ui PRIVATE ws2_32.lib setupapi.lib Winmm.lib
                                              netapi32.lib Shlwapi.lib)
else()
  find_package(X11 REQUIRED)
  target_link_libraries(aja-output-ui PRIVATE X11::X11)
endif()

set_target_properties(aja-output-ui PROPERTIES FOLDER "frontend" PREFIX "")

setup_plugin_target(aja-output-ui)
