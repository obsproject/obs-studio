project(obs-channel-matrix)

set(obs-channel-matrix_SOURCES
	obs-channel-matrix.c)

add_library(obs-channel-matrix MODULE
	${obs-channel-matrix_SOURCES})
target_link_libraries(obs-channel-matrix
	libobs)
set_target_properties(obs-channel-matrix PROPERTIES FOLDER "plugins")

install_obs_plugin(obs-channel-matrix)
