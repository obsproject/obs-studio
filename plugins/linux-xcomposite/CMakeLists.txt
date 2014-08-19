project(linux-xcomposite)

find_package(X11)
if(NOT X11_Xcomposite_FOUND)
	message(STATUS "Xcomposite library not found, Xcomposite plugin disabled")
	return()
endif()

include_directories(
	${X11_Xcomposite_INCLUDE_PATH}
	${X11_X11_INCLUDE_PATH})

set(linux-xcomposite_SOURCES
	plugin-main.cpp
	xcompcap-main.cpp
	xcompcap-main.h
	xcompcap-helper.cpp
	xcompcap-helper.h)

add_library(linux-xcomposite MODULE
	${linux-xcomposite_SOURCES})
target_link_libraries(linux-xcomposite
	libobs
	glad
	${X11_X11_LIB}
	${X11_Xcomposite_LIB})

install_obs_plugin_with_data(linux-xcomposite data)
