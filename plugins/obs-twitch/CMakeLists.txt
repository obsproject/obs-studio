cmake_minimum_required(VERSION 3.16...3.25)

add_library(obs-twitch MODULE)
add_library(OBS::obs-twitch ALIAS obs-twitch)

# Use the included GetRemoteFile function
if(NOT TARGET OBS::oauth)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/oauth" "${CMAKE_BINARY_DIR}/deps/oauth-service/oauth")
endif()

find_package(nlohmann_json)

target_sources(
  obs-twitch
  PRIVATE # cmake-format: sortable
          obs-twitch.cpp
          twitch-api.hpp
          twitch-config.cpp
          twitch-config.hpp
          twitch-service-info.cpp
          twitch-service.cpp
          twitch-service.hpp)

target_link_libraries(obs-twitch PRIVATE OBS::libobs OBS::oauth nlohmann_json::nlohmann_json)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-twitch.rc)
  target_sources(obs-twitch PRIVATE obs-twitch.rc)
endif()

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  set_target_properties_obs(obs-twitch PROPERTIES FOLDER plugins PREFIX "")
else()
  set_target_properties(obs-twitch PROPERTIES FOLDER "plugins" PREFIX "")
  setup_plugin_target(obs-twitch)
endif()
