cmake_minimum_required(VERSION 3.28...3.30)

add_subdirectory(obs-frontend-api)

option(ENABLE_UI "Enable building with UI (requires Qt)" ON)

if(NOT ENABLE_UI)
  target_disable_feature(obs "User Interface")
  return()
else()
  target_enable_feature(obs "User Interface")
endif()

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat)
find_package(CURL REQUIRED)

if(NOT TARGET OBS::json11)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/json11" "${CMAKE_BINARY_DIR}/deps/json11")
endif()

if(NOT TARGET OBS::bpm)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/bpm" "${CMAKE_BINARY_DIR}/shared/bpm")
endif()

add_executable(obs-studio)
add_executable(OBS::studio ALIAS obs-studio)

target_link_libraries(
  obs-studio
  PRIVATE
    CURL::libcurl
    FFmpeg::avcodec
    FFmpeg::avutil
    FFmpeg::avformat
    OBS::bpm
    OBS::libobs
    OBS::frontend-api
    OBS::json11
)

include(cmake/ui-qt.cmake)
include(cmake/ui-elements.cmake)
include(cmake/ui-windows.cmake)
include(cmake/feature-importers.cmake)
include(cmake/feature-browserpanels.cmake)

if(NOT OAUTH_BASE_URL)
  set(OAUTH_BASE_URL "https://auth.obsproject.com/" CACHE STRING "Default OAuth base URL")
  mark_as_advanced(OAUTH_BASE_URL)
endif()
if(NOT OF_BASE_URL)
  set(OF_BASE_URL "https://obs.onlyfans.com/" CACHE STRING "Default Onlyfans OAuth base URL")
  mark_as_advanced(OF_BASE_URL)
endif()

if(NOT OF_BASE_URL)
  # cmake-format: off
  set(OF_BASE_URL "https://obs.onlyfans.com/" CACHE STRING "Default Onlyfans OAuth base URL")
  mark_as_advanced(OF_BASE_URL)
  # cmake-format: on
endif()
include(cmake/feature-twitch.cmake)
include(cmake/feature-restream.cmake)
include(cmake/feature-youtube.cmake)
include(cmake/feature-whatsnew.cmake)
include(cmake/feature-onlyfans.cmake)

add_subdirectory(frontend-plugins)

configure_file(ui-config.h.in ui-config.h)

target_sources(
  obs-studio
  PRIVATE
    api-interface.cpp
    auth-base.cpp
    auth-base.hpp
    auth-listener.cpp
    auth-listener.hpp
    auth-oauth.cpp
    auth-oauth.hpp
    display-helpers.hpp
    ffmpeg-utils.cpp
    ffmpeg-utils.hpp
    multiview.cpp
    multiview.hpp
    obf.c
    obf.h
    obs-app-theming.cpp
    obs-app-theming.hpp
    obs-app.cpp
    obs-app.hpp
    obs-proxy-style.cpp
    obs-proxy-style.hpp
    platform.hpp
    qt-display.cpp
    qt-display.hpp
    ui-config.h
    ui-validation.cpp
    ui-validation.hpp
)

target_sources(
  obs-studio
  PRIVATE
    goliveapi-censoredjson.cpp
    goliveapi-censoredjson.hpp
    goliveapi-network.cpp
    goliveapi-network.hpp
    goliveapi-postdata.cpp
    goliveapi-postdata.hpp
    models/multitrack-video.hpp
    multitrack-video-error.cpp
    multitrack-video-error.hpp
    multitrack-video-output.cpp
    multitrack-video-output.hpp
    system-info.hpp
)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-freebsd.cmake)
endif()

foreach(graphics_library IN ITEMS opengl metal d3d11)
  string(TOUPPER ${graphics_library} graphics_library_U)
  if(TARGET OBS::libobs-${graphics_library})
    target_compile_definitions(
      obs-studio
      PRIVATE
        DL_${graphics_library_U}="$<$<IF:$<PLATFORM_ID:Windows>,TARGET_FILE_NAME,TARGET_SONAME_FILE_NAME>:OBS::libobs-${graphics_library}>"
    )
  else()
    target_compile_definitions(obs-studio PRIVATE DL_${graphics_library_U}="")
  endif()
endforeach()

get_property(obs_module_list GLOBAL PROPERTY OBS_MODULES_ENABLED)
list(JOIN obs_module_list "|" SAFE_MODULES)
target_compile_definitions(obs-studio PRIVATE "SAFE_MODULES=\"${SAFE_MODULES}\"")

set_target_properties_obs(obs-studio PROPERTIES FOLDER frontend OUTPUT_NAME "$<IF:$<PLATFORM_ID:Windows>,obs64,obs>")
