project(aja)

option(ENABLE_AJA "Build OBS with aja support" ON)
if(NOT ENABLE_AJA)
  message(STATUS "OBS:  DISABLED   aja")
  return()
endif()

if(NOT OS_MACOS AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(
    STATUS "OBS:             aja support not enabled (32-bit not supported)")
  set(ENABLE_AJA
      OFF
      CACHE BOOL "Build OBS with aja support" FORCE)
  return()
endif()

find_package(LibAJANTV2 REQUIRED)

add_library(aja MODULE)
add_library(OBS::aja ALIAS aja)

target_sources(
  aja
  PRIVATE main.cpp
          aja-card-manager.cpp
          aja-common.cpp
          aja-common.hpp
          aja-output.cpp
          aja-enums.hpp
          aja-output.hpp
          aja-props.cpp
          aja-props.hpp
          aja-routing.cpp
          aja-routing.hpp
          aja-source.cpp
          aja-source.hpp
          aja-widget-io.cpp
          aja-widget-io.hpp
          aja-card-manager.hpp
          aja-ui-props.hpp)

target_link_libraries(aja PRIVATE OBS::libobs AJA::LibAJANTV2)

if(TARGET AJA::LibAJANTV2_Debug)
  target_link_libraries(aja PRIVATE debug AJA::LibAJANTV2_Debug)
endif()

if(OS_MACOS)
  find_library(IOKIT IOKit)
  find_library(COREFOUNDATION CoreFoundation)
  find_library(APPKIT AppKit)

  target_link_libraries(aja PRIVATE ${IOKIT} ${COREFOUNDATION} ${APPKIT})
elseif(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS AJA Windows module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 win-aja.rc)

  target_sources(aja PRIVATE win-aja.rc)

  target_link_libraries(aja PRIVATE ws2_32.lib setupapi.lib Winmm.lib
                                    netapi32.lib Shlwapi.lib)
endif()

set_target_properties(aja PROPERTIES FOLDER "plugins/aja" PREFIX "")

setup_plugin_target(aja)
