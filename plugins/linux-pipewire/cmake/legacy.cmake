project(linux-pipewire)

option(ENABLE_PIPEWIRE "Enable PipeWire support" ON)
if(NOT ENABLE_PIPEWIRE)
  obs_status(DISABLED "linux-pipewire")
  return()
endif()

find_package(PipeWire 0.3.33 REQUIRED)
find_package(Gio QUIET)
find_package(Libdrm QUIET) # we require libdrm/drm_fourcc.h to build

if(NOT TARGET PipeWire::PipeWire)
  obs_status(FATAL_ERROR "PipeWire library not found! Please install PipeWire or set ENABLE_PIPEWIRE=OFF.")
elseif(NOT TARGET GIO::GIO)
  obs_status(FATAL_ERROR "Gio library not found! Please install GLib2 (or Gio) or set ENABLE_PIPEWIRE=OFF.")
elseif(NOT TARGET Libdrm::Libdrm)
  obs_status(FATAL_ERROR "libdrm headers not found! Please install libdrm or set ENABLE_PIPEWIRE=OFF.")
endif()

add_library(linux-pipewire MODULE)
add_library(OBS::pipewire ALIAS linux-pipewire)

target_sources(
  linux-pipewire
  PRIVATE formats.c
          formats.h
          linux-pipewire.c
          pipewire.c
          pipewire.h
          portal.c
          portal.h
          screencast-portal.c
          screencast-portal.h)

target_link_libraries(linux-pipewire PRIVATE OBS::libobs OBS::obsglad PipeWire::PipeWire GIO::GIO Libdrm::Libdrm)

if(PIPEWIRE_VERSION VERSION_GREATER_EQUAL 0.3.60)
  obs_status(STATUS "PipeWire 0.3.60+ found, enabling camera support")

  target_sources(linux-pipewire PRIVATE camera-portal.c camera-portal.h)
endif()

set_target_properties(linux-pipewire PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-pipewire)
