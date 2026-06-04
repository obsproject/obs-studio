cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_PIPEWIRE "Enable PipeWire support" ON)
if(NOT ENABLE_PIPEWIRE)
  target_disable(linux-pipewire)
  return()
endif()

find_package(PipeWire 0.3.33 REQUIRED)
find_package(Gio 2.76 REQUIRED)
find_package(Libdrm REQUIRED)
get_target_property(libdrm_include_directories Libdrm::Libdrm INTERFACE_INCLUDE_DIRECTORIES)

add_library(linux-pipewire MODULE)
add_library(OBS::pipewire ALIAS linux-pipewire)

if(PipeWire_VERSION VERSION_GREATER_EQUAL 0.3.60)
  target_enable_feature(linux-pipewire "PipeWire 0.3.60+ camera support")
  set(_HAS_PIPEWIRE_CAMERA TRUE)
else()
  set(_HAS_PIPEWIRE_CAMERA FALSE)
endif()

target_sources(
  linux-pipewire
  PRIVATE
    $<$<BOOL:${_HAS_PIPEWIRE_CAMERA}>:camera-portal.c>
    $<$<BOOL:${_HAS_PIPEWIRE_CAMERA}>:camera-portal.h>
    formats.c
    formats.h
    linux-pipewire.c
    pipewire.c
    pipewire.h
    portal.c
    portal.h
    screencast-portal.c
    screencast-portal.h
)

target_include_directories(linux-pipewire PRIVATE ${libdrm_include_directories})

target_link_libraries(linux-pipewire PRIVATE OBS::libobs OBS::glad PipeWire::PipeWire gio::gio)

set_target_properties_obs(linux-pipewire PROPERTIES FOLDER plugins PREFIX "")
