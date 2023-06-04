cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

option(ENABLE_UPDATER "Build with Windows updater" ON)

if(NOT ENABLE_UPDATER)
  target_disable_feature(obs "Windows updater")
  return()
endif()

find_package(zstd)

if(NOT TARGET OBS::json11)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/json11" "${CMAKE_BINARY_DIR}/deps/json11")
endif()

add_executable(updater WIN32)

target_sources(
  updater
  PRIVATE hash.cpp
          helpers.cpp
          helpers.hpp
          http.cpp
          init-hook-files.c
          patch.cpp
          resource.h
          updater.cpp
          updater.hpp
          updater.manifest
          updater.rc)

target_compile_options(updater PRIVATE $<IF:$<CONFIG:DEBUG>,/MTd,/MT> "/utf-8")
target_compile_definitions(updater PRIVATE NOMINMAX "PSAPI_VERSION=2")

target_include_directories(updater PRIVATE "${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_SOURCE_DIR}/UI/win-update")

target_link_libraries(updater PRIVATE OBS::blake2 OBS::json11 zstd::libzstd_static comctl32 shell32 winhttp)
target_link_options(updater PRIVATE /IGNORE:4098)

set_target_properties_obs(updater PROPERTIES FOLDER frontend OUTPUT_NAME updater)
