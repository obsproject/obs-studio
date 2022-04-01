project(linux-pipewire)

option(ENABLE_PIPEWIRE "Enable PipeWire support" ON)
if(NOT ENABLE_PIPEWIRE)
  obs_status(DISABLED "linux-pipewire")
  return()
endif()

find_package(PipeWire REQUIRED)
find_package(Gio QUIET)
find_package(Libdrm QUIET) # we require libdrm/drm_fourcc.h to build
find_qt(VERSION ${QT_VERSION} COMPONENTS Widgets)

if(NOT TARGET PipeWire::PipeWire)
  obs_status(
    FATAL_ERROR
    "PipeWire library not found! Please install PipeWire or set ENABLE_PIPEWIRE=OFF."
  )
elseif(NOT TARGET GIO::GIO)
  obs_status(
    FATAL_ERROR
    "Gio library not found! Please install GLib2 (or Gio) or set ENABLE_PIPEWIRE=OFF."
  )
elseif(NOT TARGET Libdrm::Libdrm)
  obs_status(
    FATAL_ERROR
    "libdrm headers not found! Please install libdrm or set ENABLE_PIPEWIRE=OFF."
  )
endif()

add_library(linux-pipewire MODULE)
add_library(OBS::pipewire ALIAS linux-pipewire)

target_sources(
  linux-pipewire
  PRIVATE linux-pipewire.c
          parent_window.cpp
          pipewire-common.c
          pipewire-common.h
          pipewire.c
          pipewire.h
          pipewire-capture.c
          pipewire-capture.h
          portal.c
          portal.h)

if(ENABLE_WAYLAND)
  find_program(WaylandScanner wayland-scanner)
  if(NOT WaylandScanner)
    obs_status(
      FATAL_ERROR
      "wayland-scanner not found! Please install wayland-scanner or set ENABLE_PIPEWIRE=OFF or set ENABLE_WAYLAND=OFF."
    )
  endif()
  add_custom_command(
    OUTPUT xdg-foreign-unstable-v2-client-protocol.h
    COMMAND
      ${WaylandScanner} --strict --include-core-only client-header
      ${CMAKE_CURRENT_SOURCE_DIR}/xdg-foreign-unstable-v2.xml
      xdg-foreign-unstable-v2-client-protocol.h)
  add_custom_command(
    OUTPUT xdg-foreign-unstable-v2-client-protocol.c
    COMMAND
      ${WaylandScanner} --strict --include-core-only private-code
      ${CMAKE_CURRENT_SOURCE_DIR}/xdg-foreign-unstable-v2.xml
      xdg-foreign-unstable-v2-client-protocol.c)
  target_include_directories(linux-pipewire PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(
    linux-pipewire PRIVATE xdg-foreign-unstable-v2-client-protocol.h
                           xdg-foreign-unstable-v2-client-protocol.c)
  target_link_libraries(linux-pipewire PRIVATE Qt::GuiPrivate)
endif()

target_link_libraries(
  linux-pipewire
  PRIVATE OBS::libobs
          OBS::obsglad
          PipeWire::PipeWire
          GIO::GIO
          Libdrm::Libdrm
          OBS::frontend-api
          Qt::Widgets)

set_target_properties(linux-pipewire PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-pipewire)
