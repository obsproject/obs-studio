project(win-wasapi)

set(win-wasapi_HEADERS
	enum-wasapi.hpp)

set(win-wasapi_SOURCES
	win-wasapi.cpp
	enum-wasapi.cpp
	plugin-main.cpp)

add_library(win-wasapi MODULE
	${win-wasapi_SOURCES}
	${win-wasapi_HEADERS})
target_link_libraries(win-wasapi
	libobs)

install_obs_plugin_with_data(win-wasapi data)
