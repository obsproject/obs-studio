project(color-source)

set(color-source_SOURCES
	color-source.c)

add_library(color-source MODULE
	${color-source_SOURCES})

target_link_libraries(color-source
	libobs
	${color-source_PLATFORM_DEPS})

install_obs_plugin_with_data(color-source data)
