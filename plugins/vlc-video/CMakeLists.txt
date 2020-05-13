project(vlc-video)

if(DISABLE_VLC)
	message(STATUS "VLC video plugin disabled")
	return()
endif()

find_package(LibVLC QUIET)

if(NOT LIBVLC_INCLUDES_FOUND AND ENABLE_VLC)
	message(FATAL_ERROR "LibVLC includes not found but set as enabled")
elseif(NOT LIBVLC_INCLUDES_FOUND)
	message(STATUS "LibVLC includes not found, VLC video plugin disabled")
	return()
endif()

include_directories(${LIBVLC_INCLUDE_DIRS})
add_definitions(${LIBVLC_DEFINITIONS})

if(MSVC)
	set(vlc-video_PLATFORM_DEPS
		w32-pthreads)
endif()

set(vlc-video_HEADERS
	vlc-video-plugin.h
	)

set(vlc-video_SOURCES
	vlc-video-plugin.c
	vlc-video-source.c
	)

if(WIN32)
	set(MODULE_DESCRIPTION "OBS VLC module")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in vlc-video.rc)
	list(APPEND vlc-video_SOURCES
		vlc-video.rc)
endif()

add_library(vlc-video MODULE
	${vlc-video_SOURCES}
	${vlc-video_HEADERS})
target_link_libraries(vlc-video
	libobs
	${vlc-video_PLATFORM_DEPS})
set_target_properties(vlc-video PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(vlc-video data)
