project(obs-x264)

find_package(Libx264 REQUIRED)

add_library(obs-x264 MODULE)
add_library(OBS::x264 ALIAS obs-x264)
add_executable(obs-x264-test)

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/opts-parser" "${CMAKE_BINARY_DIR}/shared/opts-parser")
endif()

target_sources(obs-x264-test PRIVATE obs-x264-test.c)

target_link_libraries(obs-x264-test PRIVATE OBS::opts-parser)

target_sources(obs-x264 PRIVATE obs-x264.c obs-x264-plugin-main.c)

target_link_libraries(obs-x264 PRIVATE LIBX264::LIBX264 OBS::opts-parser)

set_target_properties(obs-x264 PROPERTIES FOLDER "plugins" PREFIX "")

if(OS_WINDOWS)
  set(MODULE_DESCRIPTION "OBS x264 encoder")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in obs-x264.rc)

  target_sources(obs-x264 PRIVATE obs-x264.rc)

endif()

set_target_properties(obs-x264-test PROPERTIES FOLDER "plugins")
add_test(NAME obs-x264-test COMMAND obs-x264-test)

setup_plugin_target(obs-x264)
