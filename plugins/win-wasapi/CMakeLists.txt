project(win-wasapi)

set(ST_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/SoundTouch/include)
set(ST_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/SoundTouch/lib/SoundTouchD_x64.lib)

include_directories(${ST_INCLUDE_DIRS})

set(win-wasapi_HEADERS
	enum-wasapi.hpp
	SoundTouch-pipe.h)

set(MODULE_DESCRIPTION "OBS WASAPI module")
configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in win-wasapi.rc)
set(win-wasapi_SOURCES
	win-wasapi.cpp
	enum-wasapi.cpp
	plugin-main.cpp
	SoundTouch-pipe.cpp
	win-wasapi.rc)

add_library(win-wasapi MODULE
	${win-wasapi_SOURCES}
	${win-wasapi_HEADERS})
target_link_libraries(win-wasapi
	libobs
	${ST_LIBRARIES})
set_target_properties(win-wasapi PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(win-wasapi data)
