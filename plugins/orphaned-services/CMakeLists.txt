cmake_minimum_required(VERSION 3.16...3.25)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  find_package(jansson REQUIRED)
else()
  find_package(Jansson REQUIRED)
endif()

add_library(orphaned-services MODULE)
add_library(OBS::orphaned-services ALIAS orphaned-services)

target_sources(
  orphaned-services
  PRIVATE # cmake-format: sortable
          dacast/dacast-ingest.c
          dacast/dacast-ingest.h
          dacast/dacast.c
          dacast/dacast.h
          nimotv/nimotv-ingest.c
          nimotv/nimotv-ingest.h
          nimotv/nimotv.c
          nimotv/nimotv.h
          orphaned-services.c
          showroom/showroom-ingest.c
          showroom/showroom-ingest.h
          showroom/showroom.c
          showroom/showroom.h
          younow/younow-ingest.c
          younow/younow-ingest.h
          younow/younow.c
          younow/younow.h)

target_link_libraries(orphaned-services PRIVATE OBS::libobs OBS::file-updater)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in orphaned-services.rc)
  target_sources(orphaned-services PRIVATE orphaned-services.rc)
  target_link_options(orphaned-services PRIVATE /IGNORE:4098)
endif()

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  target_link_libraries(orphaned-services PRIVATE jansson::jansson)
  set_target_properties_obs(orphaned-services PROPERTIES FOLDER plugins PREFIX "")
else()
  target_link_libraries(orphaned-services PRIVATE Jansson::Jansson)
  set_target_properties(orphaned-services PROPERTIES FOLDER "plugins" PREFIX "")
  setup_plugin_target(orphaned-services)
endif()
