cmake_minimum_required(VERSION 3.16...3.25)

add_library(obs-youtube MODULE)
add_library(OBS::obs-youtube ALIAS obs-youtube)

target_sources(
  obs-youtube
  PRIVATE # cmake-format: sortable
          obs-youtube.cpp youtube-config.cpp youtube-config.hpp youtube-service-info.cpp youtube-service.cpp
          youtube-service.hpp)

target_link_libraries(obs-youtube PRIVATE OBS::libobs)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-youtube.rc)
  target_sources(obs-youtube PRIVATE obs-youtube.rc)
endif()

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  set_target_properties_obs(obs-youtube PROPERTIES FOLDER plugins PREFIX "")
else()
  set_target_properties(obs-youtube PROPERTIES FOLDER "plugins" PREFIX "")
  setup_plugin_target(obs-youtube)
endif()
