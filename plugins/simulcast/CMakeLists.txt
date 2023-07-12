cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

# find_package(MbedTLS REQUIRED) find_package(ZLIB REQUIRED)

add_library(simulcast MODULE)

target_sources(
  simulcast
  PRIVATE # cmake-format: sortable
          src/common.h
          src/global-service.cpp
          src/global-service.h
          src/simulcast-dock-widget.cpp
          src/simulcast-dock-widget.h
          src/simulcast-output.cpp
          src/simulcast-output.h
          src/simulcast-plugin.cpp
          src/simulcast-plugin.h)

target_link_libraries(simulcast PRIVATE OBS::libobs OBS::frontend-api Qt::Core Qt::Widgets Qt::Svg Qt::Network)

# target_compile_definitions(simulcast PRIVATE USE_MBEDTLS CRYPTO)

configure_file(src/plugin-macros.h.in plugin-macros.generated.h)
target_sources(simulcast PRIVATE plugin-macros.generated.h)

target_include_directories(simulcast PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(
  simulcast
  PROPERTIES FOLDER plugins
             PREFIX ""
             AUTOMOC ON
             AUTOUIC ON
             AUTORCC ON)

if(OS_WINDOWS)
  set_property(
    TARGET simulcast
    APPEND
    PROPERTY AUTORCC_OPTIONS --format-version 1)
endif()

# just setting this same property up above doesn't seem to be sufficient?
set_target_properties_obs(simulcast PROPERTIES FOLDER plugins PREFIX "")
