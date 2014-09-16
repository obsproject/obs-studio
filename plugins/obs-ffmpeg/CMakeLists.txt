project(obs-ffmpeg)

if(WIN32)
	set(obs-ffmpeg_PLATFORM_DEPS
		w32-pthreads)
endif()

# By default, will find ALL ffmpeg components
find_package(FFmpeg REQUIRED)
include_directories(${FFmpeg_INCLUDE_DIRS})

set(obs-ffmpeg_HEADERS
	obs-ffmpeg-formats.h
	obs-ffmpeg-compat.h)
set(obs-ffmpeg_SOURCES
	obs-ffmpeg.c
	obs-ffmpeg-aac.c
	obs-ffmpeg-output.c)
	
add_library(obs-ffmpeg MODULE
	${obs-ffmpeg_HEADERS}
	${obs-ffmpeg_SOURCES})
target_link_libraries(obs-ffmpeg
	libobs
	${obs-ffmpeg_PLATFORM_DEPS}
	${FFmpeg_LIBRARIES})

install_obs_plugin_with_data(obs-ffmpeg data)
