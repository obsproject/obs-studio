project(linux-xshm)

find_package(X11 REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux-xshm_SOURCES
	linux-xshm.c
	xcursor.c
	xhelpers.c
	xshm-input.c
)
set(linux-xshm_HEADERS
	xcursor.h
	xhelpers.h
)

add_library(linux-xshm MODULE
	${linux-xshm_SOURCES}
	${linux-xshm_HEADERS}
)
target_link_libraries(linux-xshm
	libobs
	${X11_LIBRARIES}
	${X11_XShm_LIB}
	${X11_Xfixes_LIB}
	${X11_Xinerama_LIB}
)

install_obs_plugin_with_data(linux-xshm data)
