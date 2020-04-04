project(vlc-video)

if(DISABLE_VLC)
	message(STATUS "VLC video plugin disabled")
	return()
endif()

if(ENABLE_VLC)
	find_package(LibVLC REQUIRED)
else()
	find_package(LibVLC)
	if(NOT LIBVLC_FOUND)
		message(STATUS "VLC video plugin disabled")
	endif()
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

add_library(vlc-video MODULE
	${vlc-video_SOURCES}
	${vlc-video_HEADERS})
# instead of linking vlc we load at runtime.
target_link_libraries(vlc-video
	libobs
	${vlc-video_PLATFORM_DEPS})

install_obs_plugin_with_data(vlc-video data)
