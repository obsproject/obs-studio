cmake_minimum_required(VERSION 3.16...3.25)

option(ENABLE_SERVICES_UPDATE "Checks for services update" ON)
if(ENABLE_SERVICES_UPDATE AND NOT TARGET OBS::file-updater)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/file-updater" "${CMAKE_BINARY_DIR}/deps/file-updater")
endif()

find_package(nlohmann_json)

add_library(obs-services MODULE)
add_library(OBS::obs-services ALIAS obs-services)

target_sources(
  obs-services
  PRIVATE # cmake-format: sortable
          generated/services-json.hpp
          obs-services.cpp
          service-config.cpp
          service-config.hpp
          service-instance-info.cpp
          service-instance.cpp
          service-instance.hpp
          services-json-util.hpp
          services-manager.cpp
          services-manager.hpp)

target_link_libraries(obs-services PRIVATE OBS::libobs nlohmann_json::nlohmann_json)

file(STRINGS json/services.json _format_version REGEX "^.*\"format_version\":[ \t]+[0-9]+,$")
string(REGEX REPLACE "^.*\"format_version\":[ \t]+([0-9]+),$" "\\1" JSON_FORMAT_VER "${_format_version}")

target_compile_definitions(obs-services PRIVATE JSON_FORMAT_VER=${JSON_FORMAT_VER})

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-services.rc)
  target_sources(obs-services PRIVATE obs-services.rc)
endif()

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  target_add_resource(obs-services "${CMAKE_CURRENT_SOURCE_DIR}/json/services.json"
                      "${OBS_DATA_DESTINATION}/obs-plugins/obs-services")
  set_target_properties_obs(obs-services PROPERTIES FOLDER plugins PREFIX "")
else()
  add_target_resource(obs-services json/services.json "obs-plugins/obs-services/")
  set_target_properties(obs-services PROPERTIES FOLDER "plugins" PREFIX "")
  setup_plugin_target(obs-services)
endif()
