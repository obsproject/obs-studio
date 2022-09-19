cmake_minimum_required(VERSION 3.5)
project(obs-virtualsource)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_output_suffix "64")
else()
  set(_output_suffix "32")
endif()

add_library(obs-virtualsource MODULE)
add_library(OBS::virtualcam-module ALIAS obs-virtualsource)

target_sources(
  obs-virtualsource
  PRIVATE sleepto.c
          sleepto.h
          placeholder.cpp
          virtualcam-module.cpp
          virtualcam-filter.cpp
          virtualcam-filter.hpp
          virtualcam-module.rc
          ../shared-memory-queue.c
          ../shared-memory-queue.h
          ../tiny-nv12-scale.c
          ../tiny-nv12-scale.h)

target_include_directories(obs-virtualsource
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(MODULE_DESCRIPTION "OBS Virtual Camera output module")

configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
               virtualcam-module.rc)

target_sources(obs-virtualsource PRIVATE virtualcam-module.rc)

target_link_libraries(
  obs-virtualsource
  PRIVATE OBS::libdshowcapture OBS::libdshowcapture-external setupapi winmm
          strmiids gdiplus)

target_link_options(obs-virtualsource PRIVATE "LINKER:/ignore:4104")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/virtualcam-module.def.in
               ${CMAKE_CURRENT_BINARY_DIR}/virtualcam-module.def)

target_sources(obs-virtualsource
               PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/virtualcam-module.def)

target_include_directories(obs-virtualsource
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../config)

target_compile_definitions(
  obs-virtualsource
  PRIVATE VIRTUALCAM_AVAILABLE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS
          _CRT_NONSTDC_NO_WARNINGS)

if(MSVC)
  target_compile_options(obs-virtualsource
                         PRIVATE "$<IF:$<CONFIG:Debug>,/MTd,/MT>")
  add_target_resource(win-dshow "$<TARGET_PDB_FILE:obs-virtualsource>"
                      "obs-plugins/win-dshow/" OPTIONAL)

endif()

set_target_properties(obs-virtualsource PROPERTIES FOLDER
                                                       "plugins/win-dshow")

set_target_properties(
  obs-virtualsource PROPERTIES OUTPUT_NAME
                                   "obs-virtualsource")

add_target_resource(win-dshow "$<TARGET_FILE:obs-virtualsource>"
                    "obs-plugins/win-dshow/")
