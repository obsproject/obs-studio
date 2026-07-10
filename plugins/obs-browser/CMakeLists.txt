cmake_minimum_required(VERSION 3.16...3.25)

option(ENABLE_BROWSER "Enable browser source plugin (required Chromium Embedded Framework)" OFF)

if(NOT ENABLE_BROWSER)
  target_disable(obs-browser)
  target_disable_feature(obs-browser "Browser sources are not enabled by default (set CEF_ROOT_DIR and ENABLE_BROWSER)")

  return()
endif()

find_package(CEF 149 REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)

add_library(obs-browser MODULE)
add_library(OBS::browser ALIAS obs-browser)

option(ENABLE_BROWSER_PANELS "Enable Qt web browser panel support" ON)
mark_as_advanced(ENABLE_BROWSER_PANELS)

target_sources(
  obs-browser
  PRIVATE # cmake-format: sortable
          browser-app.cpp
          browser-app.hpp
          browser-client.cpp
          browser-client.hpp
          browser-scheme.cpp
          browser-scheme.hpp
          browser-version.h
          cef-headers.hpp
          deps/base64/base64.cpp
          deps/base64/base64.hpp
          deps/ip-string.hpp
          deps/signal-restore.cpp
          deps/signal-restore.hpp
          deps/wide-string.cpp
          deps/wide-string.hpp
          obs-browser-plugin.cpp
          obs-browser-source.cpp
          obs-browser-source.hpp
          webgpu-config.cpp
          webgpu-config.hpp)

target_include_directories(obs-browser PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps")

# CEF 149 headers use C++20 concepts (for example in cef_scoped_refptr).
target_compile_features(obs-browser PRIVATE cxx_std_20)
target_link_libraries(obs-browser PRIVATE OBS::libobs OBS::frontend-api OBS::websocket-api nlohmann_json::nlohmann_json)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
endif()

if(ENABLE_BROWSER_PANELS)
  include(cmake/feature-panels.cmake)
endif()

set_target_properties_obs(obs-browser PROPERTIES FOLDER plugins/obs-browser PREFIX "")
