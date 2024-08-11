project(frontend-tools)

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

find_qt(COMPONENTS Widgets COMPONENTS_LINUX Gui)

set_target_properties(
  frontend-tools
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON
             AUTOUIC_SEARCH_PATHS "forms")

if(OS_WINDOWS)
  set_target_properties(frontend-tools PROPERTIES AUTORCC_OPTIONS "--format-version;1")
endif()

target_sources(frontend-tools PRIVATE forms/auto-scene-switcher.ui forms/captions.ui forms/output-timer.ui
                                      forms/scripts.ui)

target_sources(frontend-tools PRIVATE frontend-tools.c auto-scene-switcher.hpp auto-scene-switcher.cpp output-timer.hpp
                                      tool-helpers.hpp output-timer.cpp)

target_compile_features(frontend-tools PRIVATE cxx_std_17)

target_link_libraries(frontend-tools PRIVATE OBS::frontend-api OBS::qt-wrappers OBS::qt-plain-text-edit
                                             OBS::properties-view OBS::libobs Qt::Widgets)

if(OS_POSIX AND NOT OS_MACOS)
  target_link_libraries(frontend-tools PRIVATE Qt::GuiPrivate)
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-scripting" "${CMAKE_BINARY_DIR}/shared/obs-scripting")

if(ENABLE_SCRIPTING AND TARGET OBS::scripting)
  target_compile_definitions(frontend-tools PRIVATE ENABLE_SCRIPTING)

  target_sources(frontend-tools PRIVATE scripts.cpp scripts.hpp)

  target_link_libraries(frontend-tools PRIVATE OBS::scripting)

  if(TARGET obslua)
    target_compile_definitions(frontend-tools PRIVATE LUAJIT_FOUND)
  endif()

  if(TARGET obspython)
    target_compile_definitions(frontend-tools PRIVATE Python_FOUND)
  endif()
endif()

set_target_properties(frontend-tools PROPERTIES FOLDER "frontend" PREFIX "")

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS Frontend Tools")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in frontend-tools.rc)

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

elseif(OS_MACOS)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA)
  target_link_libraries(frontend-tools PRIVATE ${COCOA})

  target_sources(frontend-tools PRIVATE auto-scene-switcher-osx.mm)
  set_source_files_properties(auto-scene-switcher-osx.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)

elseif(OS_POSIX)
  find_package(X11 REQUIRED)

  target_link_libraries(frontend-tools PRIVATE X11::X11)

  target_sources(frontend-tools PRIVATE auto-scene-switcher-nix.cpp)
endif()

get_target_property(_SOURCES frontend-tools SOURCES)
set(_UI ${_SOURCES})
list(FILTER _UI INCLUDE REGEX ".*\\.ui?")

source_group(
  TREE "${CMAKE_CURRENT_SOURCE_DIR}/forms"
  PREFIX "UI Files"
  FILES ${_UI})
unset(_SOURCES)
unset(_UI)

setup_plugin_target(frontend-tools)
