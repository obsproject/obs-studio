include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

find_package(Libv4l2)
if(NOT LIBV4L2_FOUND)
	message(STATUS "libv4l2 not found, disabling v4l2 plugin")
	return()
endif()

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux-v4l2_SOURCES
	linux-v4l2.c
	v4l2-input.c
	v4l2-helpers.c
)

add_library(linux-v4l2 MODULE
	${linux-v4l2_SOURCES}
)
target_link_libraries(linux-v4l2
	libobs
	${LIBV4L2_LIBRARIES}
)

install_obs_plugin_with_data(linux-v4l2 data)
