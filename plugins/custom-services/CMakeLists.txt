cmake_minimum_required(VERSION 3.16...3.25)

add_library(custom-services MODULE)
add_library(OBS::custom-services ALIAS custom-services)

target_sources(
  custom-services
  PRIVATE # cmake-format: sortable
          custom-hls.c
          custom-rist.c
          custom-service.c
          custom-services.c
          custom-srt.c
          custom-whip.c
          rtmp-services.c)

target_link_libraries(custom-services PRIVATE OBS::libobs)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in custom-services.rc)
  target_sources(custom-services PRIVATE custom-services.rc)
endif()

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  set_target_properties_obs(custom-services PROPERTIES FOLDER plugins PREFIX "")
else()
  set_target_properties(custom-services PROPERTIES FOLDER "plugins" PREFIX "")
  setup_plugin_target(custom-services)
endif()
