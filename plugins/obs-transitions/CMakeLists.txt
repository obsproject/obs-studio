cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

add_library(obs-transitions MODULE)
add_library(OBS::transition ALIAS obs-transitions)

target_sources(
  obs-transitions
  PRIVATE # cmake-format: sortable
          obs-transitions.c
          transition-cut.c
          transition-fade-to-color.c
          transition-fade.c
          transition-luma-wipe.c
          transition-slide.c
          transition-stinger.c
          transition-swipe.c)

target_link_libraries(obs-transitions PRIVATE OBS::libobs)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-transitions.rc)
  target_sources(obs-transitions PRIVATE obs-transitions.rc)
endif()

# cmake-format: off
set_target_properties_obs(obs-transitions PROPERTIES FOLDER plugins PREFIX "")
# cmake-format: on
