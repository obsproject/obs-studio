project(obs-outputs)

find_package(Libx264 REQUIRED)
include_directories(${Libx264_INCLUDE_DIR})

set(obs-outputs_SOURCES
	obs-outputs.c
	rtmp-stream.c)
	
add_library(obs-outputs MODULE
	${obs-outputs_SOURCES})
target_link_libraries(obs-outputs
	libobs
	${Libx264_LIBRARIES})

install_obs_plugin(obs-outputs)

obs_fixup_install_target(obs-outputs PATH ${Libx264_LIBRARIES})
