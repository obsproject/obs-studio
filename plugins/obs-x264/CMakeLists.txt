project(obs-x264)

find_package(Libx264 REQUIRED)
include_directories(${Libx264_INCLUDE_DIR})
add_definitions(${Libx264_DEFINITIONS})

set(obs-x264_SOURCES
	obs-x264.c
	obs-x264-plugin-main.c)

add_library(obs-x264 MODULE
	${obs-x264_SOURCES})
target_link_libraries(obs-x264
	libobs
	${Libx264_LIBRARIES})

install_obs_plugin(obs-x264)

obs_fixup_install_target(obs-x264 PATH ${Libx264_LIBRARIES})
