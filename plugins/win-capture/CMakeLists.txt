cmake_minimum_required(VERSION 3.24...3.25)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  add_library(hook-config INTERFACE)
  add_library(OBS::hook-config ALIAS hook-config)
  target_sources(hook-config INTERFACE graphics-hook-ver.h graphics-hook-info.h hook-helpers.h)
  target_include_directories(hook-config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

  add_library(inject-library INTERFACE)
  add_library(OBS::inject-library ALIAS inject-library)
  target_sources(inject-library INTERFACE inject-library.c inject-library.h)
  target_include_directories(inject-library INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

  add_library(d3d8-api INTERFACE)
  add_library(OBS::d3d8-api ALIAS d3d8-api)
  target_sources(d3d8-api INTERFACE d3d8-api/d3d8.h d3d8-api/d3d8caps.h d3d8-api/d3d8types.h)
  target_include_directories(d3d8-api INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/d3d8-api")
endif()

legacy_check()

option(ENABLE_COMPAT_UPDATES "Checks for capture compatibility data updates" ON)

# cmake-format: off
set(COMPAT_URL "https://obsproject.com/obs2_update/win-capture" CACHE STRING "Default services package URL")
mark_as_advanced(COMPAT_URL)
# cmake-format: on

if(NOT TARGET OBS::ipc-util)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/ipc-util" "${CMAKE_BINARY_DIR}/deps/ipc-util")
endif()

if(NOT TARGET OBS::file-updater)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/file-updater" "${CMAKE_BINARY_DIR}/deps/file-updater")
endif()

find_package(jansson REQUIRED)

add_library(win-capture MODULE)
add_library(OBS::capture ALIAS win-capture)

target_sources(
  win-capture
  PRIVATE # cmake-format: sortable
          app-helpers.c
          app-helpers.h
          compat-format-ver.h
          compat-helpers.c
          compat-helpers.h
          cursor-capture.c
          cursor-capture.h
          dc-capture.c
          dc-capture.h
          duplicator-monitor-capture.c
          game-capture-file-init.c
          game-capture.c
          load-graphics-offsets.c
          monitor-capture.c
          nt-stuff.c
          nt-stuff.h
          plugin-main.c
          window-capture.c)

target_compile_definitions(
  win-capture PRIVATE COMPAT_URL="${COMPAT_URL}" "$<$<BOOL:${ENABLE_COMPAT_UPDATES}>:ENABLE_COMPAT_UPDATES>"
                      OBS_VERSION="${OBS_VERSION_CANONICAL}")

target_link_libraries(
  win-capture
  PRIVATE OBS::file-updater
          OBS::hook-config
          OBS::inject-library
          OBS::ipc-util
          OBS::libobs
          OBS::obfuscate
          OBS::w32-pthreads
          OBS::winrt-headers
          jansson::jansson)

target_link_options(win-capture PRIVATE /NODEFAULTLIB:msvcrtd.lib)

# cmake-format: off
set_target_properties_obs(win-capture PROPERTIES FOLDER plugins/win-capture PREFIX "")
# cmake-format: on

add_subdirectory(graphics-hook)
add_subdirectory(get-graphics-offsets)
add_subdirectory(inject-helper)
