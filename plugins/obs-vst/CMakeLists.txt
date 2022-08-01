project(obs-vst)

option(ENABLE_VST "Enable building OBS with VST plugin" ON)

if(NOT ENABLE_VST)
  message(STATUS "OBS:  DISABLED   obs-vst")
  return()
endif()

option(ENABLE_VST_BUNDLED_HEADERS "Build with Bundled Headers" ON)
mark_as_advanced(ENABLE_VST_BUNDLED_HEADERS)

add_library(obs-vst MODULE)
add_library(OBS::vst ALIAS obs-vst)

find_qt(COMPONENTS Widgets)

set_target_properties(
  obs-vst
  PROPERTIES AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

target_include_directories(
  obs-vst PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                  ${CMAKE_CURRENT_BINARY_DIR})

target_sources(
  obs-vst
  PRIVATE obs-vst.cpp VSTPlugin.cpp EditorWidget.cpp
          headers/vst-plugin-callbacks.hpp headers/EditorWidget.h
          headers/VSTPlugin.h)

target_link_libraries(obs-vst PRIVATE OBS::libobs Qt::Widgets)

target_include_directories(
  obs-vst PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/headers)

target_compile_features(obs-vst PRIVATE cxx_std_17)

if(ENABLE_VST_BUNDLED_HEADERS)
  message(STATUS "OBS:    -        obs-vst uses bundled VST headers")

  target_sources(obs-vst PRIVATE vst_header/aeffectx.h)

  target_include_directories(obs-vst
                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vst_header)
else()
  set(VST_INCLUDE_DIR
      ""
      CACHE PATH
            "Path to Steinburg headers (e.g. C:/VST3 SDK/pluginterfaces/vst2.x)"
            FORCE)
  mark_as_advanced(VST_INCLUDE_DIR)

  message(
    WARNING
      "OBS: You should only use the Steinburg headers for debugging or local builds. "
      "It is illegal to distribute the Steinburg headers with anything, and "
      "possibly against the GPL to distribute the binaries from the resultant compile."
  )

  target_sources(obs-vst PRIVATE ${VST_INCLUDE_DIR}/aeffectx.h)
endif()

if(OS_MACOS)
  find_library(FOUNDATION Foundation)
  find_library(COCOA Cocoa)
  mark_as_advanced(COCOA FOUNDATION)

  target_sources(obs-vst PRIVATE mac/VSTPlugin-osx.mm mac/EditorWidget-osx.mm)

  target_link_libraries(obs-vst PRIVATE ${COCOA} ${FOUNDATION})

elseif(OS_WINDOWS)
  target_sources(obs-vst PRIVATE win/VSTPlugin-win.cpp win/EditorWidget-win.cpp)

  target_compile_definitions(obs-vst PRIVATE UNICODE _UNICODE)

elseif(OS_POSIX)
  target_sources(obs-vst PRIVATE linux/VSTPlugin-linux.cpp
                                 linux/EditorWidget-linux.cpp)

endif()

set_target_properties(obs-vst PROPERTIES FOLDER "plugins" PREFIX "")

setup_plugin_target(obs-vst)
