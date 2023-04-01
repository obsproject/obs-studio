project(updater)

option(ENABLE_UPDATER "Build with Windows updater" ON)

if(NOT ENABLE_UPDATER)
  message(STATUS "OBS:  DISABLED   Windows updater")
  return()
endif()

find_package(zstd)

add_executable(updater WIN32)

target_sources(
  updater
  PRIVATE updater.cpp
          updater.hpp
          patch.cpp
          http.cpp
          hash.cpp
          resource.h
          updater.rc
          init-hook-files.c
          updater.manifest
          helpers.cpp
          helpers.hpp
          ${CMAKE_SOURCE_DIR}/deps/json11/json11.hpp
          ${CMAKE_SOURCE_DIR}/deps/json11/json11.cpp)

target_include_directories(updater PRIVATE ${CMAKE_SOURCE_DIR}/libobs ${CMAKE_SOURCE_DIR}/deps/json11)

target_compile_definitions(updater PRIVATE NOMINMAX "PSAPI_VERSION=2")

if(MSVC)
  target_compile_options(updater PRIVATE $<IF:$<CONFIG:DEBUG>,/MTd,/MT>)
  target_compile_options(updater PRIVATE "/utf-8")
  target_link_options(updater PRIVATE "LINKER:/IGNORE:4098")
endif()

target_link_libraries(updater PRIVATE OBS::blake2 zstd::libzstd_static comctl32 shell32 winhttp)

set_target_properties(updater PROPERTIES FOLDER "frontend")
