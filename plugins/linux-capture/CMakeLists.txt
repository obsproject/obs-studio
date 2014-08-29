project(linux-capture)

find_package(X11 REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux-capture_SOURCES
	linux-capture.c
	xcursor.c
	xhelpers.c
	capture-input.c
)
set(linux-capture_HEADERS
	xcursor.h
	xhelpers.h
)

add_library(linux-capture MODULE
	${linux-capture_SOURCES}
	${linux-capture_HEADERS}
)
target_link_libraries(linux-capture
	libobs
	${X11_LIBRARIES}
	${X11_XShm_LIB}
	${X11_Xfixes_LIB}
	${X11_Xinerama_LIB}
)

install_obs_plugin_with_data(linux-capture data)
