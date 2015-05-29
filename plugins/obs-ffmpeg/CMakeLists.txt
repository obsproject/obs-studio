project(obs-ffmpeg)

if(MSVC)
	set(obs-ffmpeg_PLATFORM_DEPS
		w32-pthreads)
endif()

find_package(FFMpeg REQUIRED
	COMPONENTS avcodec avfilter avdevice avutil swscale avformat swresample)
include_directories(${FFMPEG_INCLUDE_DIRS})

set(obs-ffmpeg_HEADERS
	obs-ffmpeg-formats.h
	obs-ffmpeg-compat.h
	closest-pixel-format.h)
set(obs-ffmpeg_SOURCES
	obs-ffmpeg.c
	obs-ffmpeg-aac.c
	obs-ffmpeg-output.c
	obs-ffmpeg-mux.c
	obs-ffmpeg-source.c)

add_library(obs-ffmpeg MODULE
	${obs-ffmpeg_HEADERS}
	${obs-ffmpeg_SOURCES})
target_link_libraries(obs-ffmpeg
	libobs
	libff
	${obs-ffmpeg_PLATFORM_DEPS}
	${FFMPEG_LIBRARIES})

install_obs_plugin_with_data(obs-ffmpeg data)

add_subdirectory(ffmpeg-mux)
