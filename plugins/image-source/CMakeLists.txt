project(image-source)

set(image-source_SOURCES
	image-source.c)

add_library(image-source MODULE
	${image-source_SOURCES})
target_link_libraries(image-source
	libobs)

install_obs_plugin_with_data(image-source data)
