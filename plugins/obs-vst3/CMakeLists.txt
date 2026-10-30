cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_VST3 "Enable building OBS with VST3 plugin" ON)

if(NOT ENABLE_VST3)
  target_disable(obs-vst3)
  return()
endif()

find_package(Qt6 REQUIRED Widgets)

find_package(SteinbergVST3 REQUIRED)

include(cmake/obs-vst3-library.cmake)
add_subdirectory(vst3-scan)

add_library(obs-vst3 MODULE)
add_library(OBS::vst3 ALIAS obs-vst3)

target_sources(
  obs-vst3
  PRIVATE
    $<$<PLATFORM_ID:Linux>:RunLoopImpl.cpp>
    VST3ComponentHolder.cpp
    VST3ComponentHolder.h
    VST3EditorWindow.cpp
    VST3EditorWindow.h
    VST3HostApp.cpp
    VST3HostApp.h
    VST3Plugin.cpp
    VST3Plugin.h
    obs-vst3.cpp
    obs-vst3.h
    plugin-main.cpp
)

if(OS_MACOS)
  set_source_files_properties(VST3EditorWindow.cpp PROPERTIES LANGUAGE OBJCXX COMPILE_OPTIONS "-fobjc-arc")
endif()

target_link_libraries(
  obs-vst3
  PRIVATE
    OBS::frontend-api
    OBS::libobs
    OBS::vst3-library
    Qt6::Widgets
    Steinberg::VST3
    $<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>
    $<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Foundation.framework>>
)

add_dependencies(obs-vst3 obs-vst3-scanner)

# Windows resources
if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-vst3.rc)
  target_sources(obs-vst3 PRIVATE obs-vst3.rc)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/obs-studio.ico
    ${CMAKE_CURRENT_BINARY_DIR}/obs-studio.ico
    COPYONLY
  )
endif()

set_target_properties_obs(obs-vst3 PROPERTIES FOLDER plugins PREFIX "" AUTOMOC ON)
