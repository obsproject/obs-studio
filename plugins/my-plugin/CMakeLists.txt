project(my-plugin)


set(my-plugin_SOURCES
	my-plugin.cpp)

add_library(my-plugin MODULE
	${my-plugin_SOURCES})

target_link_libraries(my-plugin
	libobs
	${my-plugin_PLATFORM_DEPS})

install_obs_plugin_with_data(my-plugin data)
