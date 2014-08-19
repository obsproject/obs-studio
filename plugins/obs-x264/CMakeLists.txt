project(obs-x264)

find_package(Libx264 REQUIRED)
include_directories(${LIBX264_INCLUDE_DIRS})
add_definitions(${LIBX264_DEFINITIONS})

set(obs-x264_SOURCES
	obs-x264.c
	obs-x264-plugin-main.c)

add_library(obs-x264 MODULE
	${obs-x264_SOURCES})
target_link_libraries(obs-x264
	libobs
	${LIBX264_LIBRARIES})

install_obs_plugin_with_data(obs-x264 data)
