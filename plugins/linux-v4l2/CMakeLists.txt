include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(linux-v4l2_SOURCES
	linux-v4l2.c
	v4l2-input.c
)

add_library(linux-v4l2 MODULE
	${linux-v4l2_SOURCES}
)
target_link_libraries(linux-v4l2
	libobs
)

install_obs_plugin(linux-v4l2)
