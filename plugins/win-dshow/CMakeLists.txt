project(win-dshow)

find_package(Libavutil REQUIRED)
include_directories(${LIBAVUTIL_INCLUDE_DIRS})
add_definitions(${LIBAVUTIL_DEFINITIONS})

find_package(Libavcodec REQUIRED)
include_directories(${LIBAVCODEC_INCLUDE_DIRS})
add_definitions(${LIBAVCODEC_DEFINITIONS})

set(win-dshow_HEADERS
	ffmpeg-decode.h)

set(win-dshow_SOURCES
	win-dshow.cpp
	ffmpeg-decode.c)

set(libdshowcapture_SOURCES
	libdshowcapture/source/capture-filter.cpp
	libdshowcapture/source/dshowcapture.cpp
	libdshowcapture/source/device.cpp
	libdshowcapture/source/dshow-base.cpp
	libdshowcapture/source/dshow-demux.cpp
	libdshowcapture/source/dshow-enum.cpp
	libdshowcapture/source/dshow-formats.cpp
	libdshowcapture/source/dshow-media-type.cpp
	libdshowcapture/source/dshow-encoded-device.cpp
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
	${win-dshow_HEADERS}
	${libdshowcapture_SOURCES}
	${libdshowcapture_HEADERS})
target_link_libraries(win-dshow
	libobs
	strmiids.lib
	${LIBAVUTIL_LIBRARIES}
	${LIBAVCODEC_LIBRARIES})

install_obs_plugin_with_data(win-dshow data)
