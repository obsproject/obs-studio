project(win-wasapi)

set(win-wasapi_HEADERS
	${CMAKE_SOURCE_DIR}/deps/was-notify/audio-device-event.h
	enum-wasapi.hpp)
	
set(MODULE_DESCRIPTION "OBS WASAPI module")
configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in win-wasapi.rc)
set(win-wasapi_SOURCES
	win-wasapi.cpp
	${CMAKE_SOURCE_DIR}/deps/was-notify/audio-device-event.cpp
	enum-wasapi.cpp
	plugin-main.cpp
	win-wasapi.rc)

add_library(win-wasapi MODULE
	${win-wasapi_SOURCES}
	${win-wasapi_HEADERS})
target_link_libraries(win-wasapi
	libobs)
set_target_properties(win-wasapi PROPERTIES FOLDER "plugins")
target_include_directories(win-wasapi PUBLIC ${CMAKE_SOURCE_DIR}/deps/)

install_obs_plugin_with_data(win-wasapi data)
