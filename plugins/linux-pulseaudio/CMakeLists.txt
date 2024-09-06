cmake_minimum_required(VERSION 3.22...3.25)

if(NOT ENABLE_PULSEAUDIO)
  target_disable(linux-pulseaudio)
  return()
endif()

find_package(PulseAudio REQUIRED)

add_library(linux-pulseaudio MODULE)
add_library(OBS::pulseaudio ALIAS linux-pulseaudio)

target_sources(linux-pulseaudio PRIVATE linux-pulseaudio.c pulse-input.c pulse-wrapper.c)
target_link_libraries(linux-pulseaudio PRIVATE OBS::libobs PulseAudio::PulseAudio)

set_target_properties_obs(linux-pulseaudio PROPERTIES FOLDER plugins PREFIX "")
