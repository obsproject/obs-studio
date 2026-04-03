cmake_minimum_required(VERSION 3.28...3.30)

add_library(obs-uvmap-filter MODULE)
add_library(OBS::uvmap-filter ALIAS obs-uvmap-filter)

target_sources(
  obs-uvmap-filter
  PRIVATE obs-uvmap-filter.c uvmap-filter.c)

target_link_libraries(obs-uvmap-filter PRIVATE OBS::libobs
                                               $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-uvmap-filter.rc)
  target_sources(obs-uvmap-filter PRIVATE obs-uvmap-filter.rc)
endif()

set_target_properties_obs(obs-uvmap-filter PROPERTIES FOLDER plugins PREFIX "")
