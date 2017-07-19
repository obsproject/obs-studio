project(obs-transitions)

set(obs-transitions_SOURCES
	obs-transitions.c
	transition-slide.c
	transition-swipe.c
	transition-fade.c
	transition-cut.c
	transition-fade-to-color.c
	transition-luma-wipe.c
	transition-stinger.c
	)

add_library(obs-transitions MODULE
	${obs-transitions_SOURCES})
target_link_libraries(obs-transitions
	libobs)

install_obs_plugin_with_data(obs-transitions data)
