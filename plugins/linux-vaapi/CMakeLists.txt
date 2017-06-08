project(linux-vaapi)

find_package(LibVa)
find_package(X11 REQUIRED)

include_directories(
	${LIBVA_INCLUDE_DIRS}
)

set(linux-vaapi_HEADERS
	bitstream.h
	vaapi-internal.h
	surface-queue.h
	vaapi-caps.h
	vaapi-common.h
	vaapi-display.h
	vaapi-encoder.h)

set(linux-vaapi_SOURCES
	bitstream.c
	surface-queue.c
	vaapi-caps.c
	vaapi-common.c
	vaapi-display.c
	vaapi-encoder.c
	vaapi-plugin.c
)

add_library(linux-vaapi MODULE
	${linux-vaapi_SOURCES}
)

target_link_libraries(linux-vaapi
	libobs
	${LIBVA_LIBRARIES}
	${X11_LIBRARIES}
	${X11_X11_LIB}
	${X11_Xfixes_LIB})

install_obs_plugin_with_data(linux-vaapi data)
