project(linux-pulseaudio)

if(NOT ENABLE_PULSEAUDIO)
  obs_status(DISABLED "linux-pulseaudio")
  return()
endif()

find_package(PulseAudio REQUIRED)

add_library(linux-pulseaudio MODULE)
add_library(OBS::pulseaudio ALIAS linux-pulseaudio)

target_sources(linux-pulseaudio PRIVATE linux-pulseaudio.c pulse-wrapper.c pulse-input.c)

target_include_directories(linux-pulseaudio PRIVATE ${PULSEAUDIO_INCLUDE_DIR})

target_link_libraries(linux-pulseaudio PRIVATE OBS::libobs ${PULSEAUDIO_LIBRARY})

set_target_properties(linux-pulseaudio PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-pulseaudio)
