cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_SNDIO "Build OBS with sndio support" OFF)
if(NOT ENABLE_SNDIO)
  target_disable(sndio)
  return()
endif()

find_package(Sndio REQUIRED)

add_library(sndio MODULE)
add_library(OBS::sndio ALIAS sndio)

target_sources(sndio PRIVATE sndio.c sndio-input.c)

target_compile_options(sndio PRIVATE -Wno-error=vla)

target_link_libraries(sndio PRIVATE OBS::libobs Sndio::Sndio)

set_target_properties_obs(sndio PROPERTIES FOLDER plugins PREFIX "")
