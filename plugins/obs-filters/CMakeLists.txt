project(obs-filters)

set(obs-filters_SOURCES
	obs-filters.c
	async-delay-filter.c
	mask-filter.c)

add_library(obs-filters MODULE
	${obs-filters_SOURCES})
target_link_libraries(obs-filters
	libobs)

install_obs_plugin_with_data(obs-filters data)
