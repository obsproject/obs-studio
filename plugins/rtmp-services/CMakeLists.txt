project(rtmp-services)

include_directories(${OBS_JANSSON_INCLUDE_DIRS})

set(rtmp-services_SOURCES
	rtmp-common.c
	rtmp-custom.c
	rtmp-services-main.c)

add_library(rtmp-services MODULE
	${rtmp-services_SOURCES})
target_link_libraries(rtmp-services
	libobs
	${OBS_JANSSON_IMPORT})

install_obs_plugin(rtmp-services)
install_obs_plugin_data(rtmp-services ../../build/data/obs-plugins/rtmp-services)
