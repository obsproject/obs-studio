project(win-capture)

set(win-capture_HEADERS
	dc-capture.h)

set(win-capture_SOURCES
	dc-capture.c
	monitor-capture.c
	plugin-main.c)

add_library(win-capture MODULE
	${win-capture_SOURCES}
	${win-capture_HEADERS})
target_link_libraries(win-capture
	libobs)

install_obs_plugin(win-capture)
install_obs_plugin_data(win-capture ../../build/data/obs-plugins/win-capture)
