project(decklink-output-ui)

if(NOT ENABLE_DECKLINK)
  return()
endif()

if(NOT COMMAND find_qt)
  macro(find_qt)
    set(oneValueArgs VERSION)
    set(multiValueArgs COMPONENTS COMPONENTS_WIN COMPONENTS_MAC
                       COMPONENTS_LINUX)
    cmake_parse_arguments(FIND_QT "" "${oneValueArgs}" "${multiValueArgs}"
                          ${ARGN})

    if(OS_WINDOWS)
      find_package(
        Qt${FIND_QT_VERSION}
        COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_WIN}
        REQUIRED)
    elseif(OS_MACOS)
      find_package(
        Qt${FIND_QT_VERSION}
        COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_MAC}
        REQUIRED)
    else()
      find_package(
        Qt${FIND_QT_VERSION}
        COMPONENTS ${FIND_QT_COMPONENTS} ${FIND_QT_COMPONENTS_LINUX}
        REQUIRED)
    endif()

    foreach(_COMPONENT IN LISTS ${FIND_QT_COMPONENTS})
      if(NOT TARGET Qt::${_COMPONENT} AND TARGET
                                          Qt${FIND_QT_VERSION}::${_COMPONENT})
        add_library(Qt::${_COMPONENT} ALIAS Qt${FIND_QT_VERSION}::${_COMPONENT})
      endif()
    endforeach()
  endmacro()
endif()

add_library(decklink-output-ui MODULE)
add_library(OBS::decklink-output-ui ALIAS decklink-output-ui)

find_qt(
  VERSION ${QT_VERSION}
  COMPONENTS Widgets
  COMPONENTS_LINUX Gui)

set_target_properties(
  decklink-output-ui
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS "forms")

target_sources(
  decklink-output-ui
  PRIVATE DecklinkOutputUI.cpp
          DecklinkOutputUI.h
          decklink-ui-main.cpp
          decklink-ui-main.h
          ${CMAKE_SOURCE_DIR}/UI/combobox-ignorewheel.cpp
          ${CMAKE_SOURCE_DIR}/UI/combobox-ignorewheel.hpp
          ${CMAKE_SOURCE_DIR}/UI/double-slider.cpp
          ${CMAKE_SOURCE_DIR}/UI/double-slider.hpp
          ${CMAKE_SOURCE_DIR}/UI/properties-view.hpp
          ${CMAKE_SOURCE_DIR}/UI/properties-view.cpp
          ${CMAKE_SOURCE_DIR}/UI/properties-view.moc.hpp
          ${CMAKE_SOURCE_DIR}/UI/qt-wrappers.hpp
          ${CMAKE_SOURCE_DIR}/UI/qt-wrappers.cpp
          ${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.cpp
          ${CMAKE_SOURCE_DIR}/UI/spinbox-ignorewheel.hpp
          ${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.cpp
          ${CMAKE_SOURCE_DIR}/UI/slider-ignorewheel.hpp
          ${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.hpp
          ${CMAKE_SOURCE_DIR}/UI/vertical-scroll-area.cpp)

target_link_libraries(decklink-output-ui PRIVATE OBS::libobs OBS::frontend-api
                                                 Qt::Widgets)

target_compile_features(decklink-output-ui PRIVATE cxx_std_17)

set_target_properties(decklink-output-ui PROPERTIES FOLDER "frontend" PREFIX "")

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS Decklink Output UI")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 decklink-output-ui.rc)

  target_sources(decklink-output-ui PRIVATE decklink-output-ui.rc)

elseif(OS_MACOS)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA)

  target_link_libraries(decklink-output-ui PRIVATE ${COCOA})

elseif(OS_POSIX)
  find_package(X11 REQUIRED)
  target_link_libraries(decklink-output-ui PRIVATE X11::X11
                                                   Qt${QT_VERSION}::GuiPrivate)
endif()

setup_plugin_target(decklink-output-ui)
