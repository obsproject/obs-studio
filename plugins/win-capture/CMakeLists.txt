cmake_minimum_required(VERSION 3.24...3.25)

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

if(NOT TARGET OBS::inject-library)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-inject-library" "${CMAKE_BINARY_DIR}/shared/obs-inject-library")
endif()

option(ENABLE_COMPAT_UPDATES "Checks for capture compatibility data updates" ON)

set(COMPAT_URL "https://obsproject.com/obs2_update/win-capture" CACHE STRING "Default services package URL")
mark_as_advanced(COMPAT_URL)

if(NOT TARGET OBS::ipc-util)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/ipc-util" "${CMAKE_BINARY_DIR}/shared/ipc-util")
endif()

if(NOT TARGET OBS::file-updater)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/file-updater" "${CMAKE_BINARY_DIR}/shared/file-updater")
endif()

find_package(jansson REQUIRED)

add_library(win-capture MODULE)
add_library(OBS::capture ALIAS win-capture)

target_sources(
  win-capture
  PRIVATE
    app-helpers.c
    app-helpers.h
    audio-helpers.c
    audio-helpers.h
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
    window-capture.c
)

target_compile_definitions(
  win-capture
  PRIVATE
    COMPAT_URL="${COMPAT_URL}"
    "$<$<BOOL:${ENABLE_COMPAT_UPDATES}>:ENABLE_COMPAT_UPDATES>"
    OBS_VERSION="${OBS_VERSION_CANONICAL}"
)

target_link_libraries(
  win-capture
  PRIVATE
    OBS::file-updater
    OBS::hook-config
    OBS::inject-library
    OBS::ipc-util
    OBS::libobs
    OBS::obfuscate
    OBS::w32-pthreads
    OBS::winrt-headers
    jansson::jansson
)

# Remove once jansson has been fixed on obs-deps
target_link_options(win-capture PRIVATE /IGNORE:4098)

set_target_properties_obs(win-capture PROPERTIES FOLDER plugins/win-capture PREFIX "")

add_subdirectory(graphics-hook)
add_subdirectory(get-graphics-offsets)
add_subdirectory(inject-helper)
