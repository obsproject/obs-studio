project(obs-x264)

find_package(Libx264 REQUIRED)
include_directories(${LIBX264_INCLUDE_DIRS})
add_definitions(${LIBX264_DEFINITIONS})

set(obs-x264_SOURCES
	obs-x264.c
	obs-x264-plugin-main.c)

if(WIN32)
	set(MODULE_DESCRIPTION "OBS x264 encoder")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in obs-x264.rc)
	list(APPEND obs-x264_SOURCES
		obs-x264.rc)
endif()

add_library(obs-x264 MODULE
	${obs-x264_SOURCES})
target_link_libraries(obs-x264
	libobs
	${LIBX264_LIBRARIES})
set_target_properties(obs-x264 PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(obs-x264 data)
