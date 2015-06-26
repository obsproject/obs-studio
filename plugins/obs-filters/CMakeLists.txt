project(obs-filters)

set(obs-filters_SOURCES
	obs-filters.c
	color-filter.c
	async-delay-filter.c
	crop-filter.c
	scroll-filter.c
	chroma-key-filter.c
	color-key-filter.c
	sharpness-filter.c
	gain-filter.c
	noise-gate-filter.c
	mask-filter.c)

add_library(obs-filters MODULE
	${obs-filters_SOURCES})
target_link_libraries(obs-filters
	libobs)

install_obs_plugin_with_data(obs-filters data)
