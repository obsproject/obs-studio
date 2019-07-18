project(obs-track-out)

set(obs-track-out_HEADERS
	)

set(obs-track-out_SOURCES
	obs-track-out.c)

add_library(obs-track-out MODULE
	${obs-track-out_SOURCES}
	${obs-track-out_HEADERS})
target_link_libraries(obs-track-out
	libobs)

install_obs_plugin(obs-track-out)