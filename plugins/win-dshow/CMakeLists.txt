if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libdshowcapture/dshowcapture.hpp")
	message(STATUS "libdshowcapture submodule not found!  Please fetch submodules.  win-dshow plugin disabled.")
	return()
endif()

project(win-dshow)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)
include_directories(${FFMPEG_INCLUDE_DIRS})

set(win-dshow_HEADERS
	encode-dstr.hpp
	ffmpeg-decode.h)

set(MODULE_DESCRIPTION "OBS DirectShow module")
configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in win-dshow.rc)
set(win-dshow_SOURCES
	win-dshow.cpp
	win-dshow-encoder.cpp
	dshow-plugin.cpp
	ffmpeg-decode.c
	win-dshow.rc)

set(libdshowcapture_SOURCES
	libdshowcapture/source/capture-filter.cpp
	libdshowcapture/source/output-filter.cpp
	libdshowcapture/source/dshowcapture.cpp
	libdshowcapture/source/dshowencode.cpp
	libdshowcapture/source/device.cpp
	libdshowcapture/source/encoder.cpp
	libdshowcapture/source/dshow-base.cpp
	libdshowcapture/source/dshow-demux.cpp
	libdshowcapture/source/dshow-enum.cpp
	libdshowcapture/source/dshow-formats.cpp
	libdshowcapture/source/dshow-media-type.cpp
	libdshowcapture/source/dshow-encoded-device.cpp
	libdshowcapture/source/log.cpp)

set(libdshowcapture_HEADERS
	libdshowcapture/dshowcapture.hpp
	libdshowcapture/source/external/IVideoCaptureFilter.h
	libdshowcapture/source/capture-filter.hpp
	libdshowcapture/source/output-filter.hpp
	libdshowcapture/source/device.hpp
	libdshowcapture/source/encoder.hpp
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
	strmiids
	ksuser
	wmcodecdspuuid
	${FFMPEG_LIBRARIES})
set_target_properties(win-dshow PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(win-dshow data)
