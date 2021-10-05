project(linux-pipewire)

option(ENABLE_PIPEWIRE "Enable PipeWire support" ON)
if(NOT ENABLE_PIPEWIRE)
  message(STATUS "PipeWire support disabled, linux-pipewire plugin disabled")
  return()
endif()

find_package(PipeWire REQUIRED)

if(NOT TARGET PipeWire::PipeWire)
  message(
    FATAL_ERROR
      "OBS:    -        PipeWire library not found! Please install PipeWire or set ENABLE_PIPEWIRE=OFF"
  )
endif()

add_library(linux-pipewire MODULE)
add_library(OBS::pipewire ALIAS linux-pipewire)

target_sources(linux-pipewire PRIVATE linux-pipewire.c pipewire-common.c
                                      pipewire-common.h)

target_link_libraries(linux-pipewire PRIVATE OBS::libobs PipeWire::PipeWire)

set_target_properties(linux-pipewire PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-pipewire)
