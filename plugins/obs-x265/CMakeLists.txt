project(obs-x265)

find_package(Libx265 REQUIRED)
include_directories(${LIBX264_INCLUDE_DIRS})
add_definitions(${LIBX264_DEFINITIONS})

set(obs-x265-util_HEADERS
	obs-x265-options.h)

set(obs-x265-util_SOURCES
	obs-x265-options.c)

add_library(obs-x265-util STATIC
	${obs-x265-util_HEADERS}
	${obs-x265-util_SOURCES})
target_link_libraries(obs-x265-util PRIVATE libobs)
set_target_properties(obs-x265-util PROPERTIES
	FOLDER "plugins"
	POSITION_INDEPENDENT_CODE ON)

set(obs-x265_SOURCES
	obs-x265.c
	obs-x265-plugin-main.c)

if(WIN32)
	set(MODULE_DESCRIPTION "OBS x265 encoder")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in obs-x265.rc)
	list(APPEND obs-x265_SOURCES
		obs-x265.rc)
endif()

add_library(obs-x265 MODULE
	${obs-x265_HEADERS}
	${obs-x265_SOURCES})
target_link_libraries(obs-x265
	libobs
	obs-x265-util
	${LIBX264_LIBRARIES})
set_target_properties(obs-x265 PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(obs-x265 data)

add_executable(obs-x265-test obs-x265-test.c)
set_target_properties(obs-x265-test PROPERTIES FOLDER "plugins")
target_link_libraries(obs-x265-test PRIVATE libobs obs-x265-util)
add_test(NAME obs-x265-test COMMAND obs-x265-test)
