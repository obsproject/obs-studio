project(obs-outputs)

find_package(Libx264 REQUIRED)
include_directories(${Libx264_INCLUDE_DIR})

set(obs-outputs_SOURCES
	obs-outputs.c
	obs-x264.c
	rtmp-stream.c)

set(obs-outputs_HEADERS
	obs-outputs.h
	obs-x264.h
	rtmp-stream.h)
	
add_library(obs-outputs MODULE
	${obs-outputs_SOURCES}
	${obs-outputs_HEADERS})
target_link_libraries(obs-outputs
	libobs
	${Libx264_LIBRARIES})

install_obs_plugin(obs-outputs)
