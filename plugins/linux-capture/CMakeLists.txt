project(linux-capture)

find_package(X11 REQUIRED)
if(NOT X11_Xcomposite_FOUND)
	message(STATUS "Xcomposite library not found, linux-capture plugin disabled")
	return()
endif()

include_directories(SYSTEM
	"${CMAKE_SOURCE_DIR}/libobs"
	${X11_Xcomposite_INCLUDE_PATH}
	${X11_X11_INCLUDE_PATH}
) 

set(linux-capture_SOURCES
	linux-capture.c
	xcursor.c
	xhelpers.c
	xshm-input.c
	xcomposite-main.cpp
	xcompcap-main.cpp
	xcompcap-helper.cpp
)
set(linux-capture_HEADERS
	xcursor.h
	xhelpers.h
	xcompcap-main.hpp
	xcompcap-helper.hpp
)

add_library(linux-capture MODULE
	${linux-capture_SOURCES}
	${linux-capture_HEADERS}
)
target_link_libraries(linux-capture
	libobs
	glad
	${X11_LIBRARIES}
	${X11_XShm_LIB}
	${X11_Xfixes_LIB}
	${X11_Xinerama_LIB}
	${X11_X11_LIB}
	${X11_Xcomposite_LIB}
)

install_obs_plugin_with_data(linux-capture data)
