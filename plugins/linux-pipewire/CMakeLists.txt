project(linux-pipewire)

option(ENABLE_PIPEWIRE "Enable PipeWire support" ON)
if(NOT ENABLE_PIPEWIRE)
  message(STATUS "PipeWire support disabled, linux-pipewire plugin disabled")
  return()
endif()

find_package(PipeWire REQUIRED)
find_package(Gio QUIET)
find_package(Libdrm QUIET) # we require libdrm/drm_fourcc.h to build

if(NOT TARGET PipeWire::PipeWire)
  message(
    FATAL_ERROR
      "OBS:    -        PipeWire library not found! Please install PipeWire or set ENABLE_PIPEWIRE=OFF"
  )
elseif(NOT TARGET GIO::GIO)
  message(
    FATAL_ERROR
      "OBS:    -        Gio library not found! Please install GLib2 (or Gio) or set ENABLE_PIPEWIRE=OFF"
  )
elseif(NOT TARGET Libdrm::Libdrm)
  message(
    FATAL_ERROR
      "OBS:    -        libdrm headers not found! Please install libdrm or set ENABLE_PIPEWIRE=OFF"
  )
endif()

add_library(linux-pipewire MODULE)
add_library(OBS::pipewire ALIAS linux-pipewire)

target_sources(
  linux-pipewire
  PRIVATE linux-pipewire.c
          pipewire-common.c
          pipewire-common.h
          pipewire.c
          pipewire.h
          pipewire-capture.c
          pipewire-capture.h
          portal.c
          portal.h)

target_link_libraries(
  linux-pipewire PRIVATE OBS::libobs OBS::obsglad PipeWire::PipeWire GIO::GIO
                         Libdrm::Libdrm)

set_target_properties(linux-pipewire PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-pipewire)
