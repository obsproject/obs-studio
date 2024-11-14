cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_ALSA "Build OBS with ALSA support" ON)

if(NOT ENABLE_ALSA)
  target_disable(linux-alsa)
  return()
endif()

find_package(ALSA REQUIRED)

add_library(linux-alsa MODULE)
add_library(OBS::alsa ALIAS linux-alsa)

target_sources(linux-alsa PRIVATE # cmake-format: sortable
                                  alsa-input.c linux-alsa.c)
target_link_libraries(linux-alsa PRIVATE OBS::libobs ALSA::ALSA)

set_target_properties_obs(linux-alsa PROPERTIES FOLDER plugins PREFIX "")
