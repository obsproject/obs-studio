cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

find_package(zstd)
find_package(nlohmann_json 3 REQUIRED)

add_executable(updater WIN32)

target_sources(
  updater
  PRIVATE hash.cpp
          helpers.cpp
          helpers.hpp
          http.cpp
          init-hook-files.c
          manifest.hpp
          patch.cpp
          resource.h
          updater.cpp
          updater.hpp
          updater.manifest
          updater.rc)

target_compile_definitions(updater PRIVATE NOMINMAX "PSAPI_VERSION=2")

target_include_directories(updater PRIVATE "${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_SOURCE_DIR}/UI/win-update")

target_link_libraries(updater PRIVATE OBS::blake2 nlohmann_json::nlohmann_json zstd::libzstd_static comctl32 shell32
                                      winhttp)

# zstd is hardcoded with /DEFAULTLIB:LIBCMT
target_link_options(updater PRIVATE /NODEFAULTLIB:LIBCMT)

set_target_properties(updater PROPERTIES FOLDER frontend OUTPUT_NAME updater)
