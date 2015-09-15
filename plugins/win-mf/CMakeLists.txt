project(win-mf)

set(win-mf_SOURCES
	mf-plugin.cpp
	mf-aac.cpp
	mf-aac-encoder.cpp
	mf-common.cpp
	mf-encoder-descriptor.cpp
	mf-h264.cpp
	mf-h264-encoder.cpp)

set(win-mf_HEADERS
	mf-common.hpp
	mf-encoder-descriptor.hpp
	mf-aac-encoder.hpp
	mf-h264-encoder.hpp)

add_library(win-mf MODULE
	${win-mf_SOURCES}
	${win-mf_HEADERS})

target_link_libraries(win-mf
	d3d9
	dxva2
	uuid
	mfplat
	mfuuid
	mf
	wmcodecdspuuid
	libobs)

install_obs_plugin_with_data(win-mf data)
