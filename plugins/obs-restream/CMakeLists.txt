cmake_minimum_required(VERSION 3.16...3.25)

add_library(obs-restream MODULE)
add_library(OBS::obs-restream ALIAS obs-restream)

# Use the included GetRemoteFile function
if(NOT TARGET OBS::oauth)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/oauth" "${CMAKE_BINARY_DIR}/deps/oauth-service/oauth")
endif()

find_package(nlohmann_json)

target_sources(
  obs-restream
  PRIVATE # cmake-format: sortable
          obs-restream.cpp
          restream-api.hpp
          restream-config.cpp
          restream-config.hpp
          restream-service-info.cpp
          restream-service.cpp
          restream-service.hpp)

target_link_libraries(obs-restream PRIVATE OBS::libobs OBS::oauth nlohmann_json::nlohmann_json)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-restream.rc)
  target_sources(obs-restream PRIVATE obs-restream.rc)
endif()

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  set_target_properties_obs(obs-restream PROPERTIES FOLDER plugins PREFIX "")
else()
  set_target_properties(obs-restream PROPERTIES FOLDER "plugins" PREFIX "")
  setup_plugin_target(obs-restream)
endif()
