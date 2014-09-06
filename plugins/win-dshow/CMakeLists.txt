project(win-dshow)

set(win-dshow_SOURCES
	win-dshow.cpp)

set(libdshowcapture_SOURCES
	libdshowcapture/source/capture-filter.cpp
	libdshowcapture/source/dshowcapture.cpp
	libdshowcapture/source/device.cpp
	libdshowcapture/source/dshow-base.cpp
	libdshowcapture/source/dshow-demux.cpp
	libdshowcapture/source/dshow-enum.cpp
	libdshowcapture/source/dshow-formats.cpp
	libdshowcapture/source/dshow-hd-pvr2.cpp
	libdshowcapture/source/dshow-hd-pvr-rocket.cpp
	libdshowcapture/source/dshow-media-type.cpp
	libdshowcapture/source/dshow-roxio.cpp
	libdshowcapture/source/log.cpp)

set(libdshowcapture_HEADERS
	libdshowcapture/dshowcapture.hpp
	libdshowcapture/source/IVideoCaptureFilter.h
	libdshowcapture/source/capture-filter.hpp
	libdshowcapture/source/device.hpp
	libdshowcapture/source/dshow-base.hpp
	libdshowcapture/source/dshow-demux.hpp
	libdshowcapture/source/dshow-device-defs.hpp
	libdshowcapture/source/dshow-enum.hpp
	libdshowcapture/source/dshow-formats.hpp
	libdshowcapture/source/dshow-media-type.hpp
	libdshowcapture/source/log.hpp)

add_library(win-dshow MODULE
	${win-dshow_SOURCES}
	${libdshowcapture_SOURCES}
	${libdshowcapture_HEADERS})
target_link_libraries(win-dshow
	libobs
	strmiids.lib)

install_obs_plugin_with_data(win-dshow data)
