project(dshow)

set(dshow_SOURCES
	dshow-plugin.cpp)

set(dshow_HEADERS
	dshow-plugin.hpp)
	
add_library(dshow MODULE
	${dshow_SOURCES}
	${dshow_HEADERS})
target_link_libraries(dshow
	libobs)

install_obs_plugin(dshow)
