project(win-capture)

set(win-capture_HEADERS
	dc-capture.h)

set(win-capture_SOURCES
	dc-capture.c
	monitor-capture.c
	window-capture.c
	plugin-main.c)

add_library(win-capture MODULE
	${win-capture_SOURCES}
	${win-capture_HEADERS})
target_link_libraries(win-capture
	libobs
	psapi.lib)

install_obs_plugin_with_data(win-capture data)
