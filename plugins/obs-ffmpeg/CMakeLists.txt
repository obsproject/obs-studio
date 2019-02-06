project(obs-ffmpeg)

if(MSVC)
	set(obs-ffmpeg_PLATFORM_DEPS
		w32-pthreads)
endif()

find_package(FFmpeg REQUIRED
	COMPONENTS avcodec avfilter avdevice avutil swscale avformat swresample)
include_directories(${FFMPEG_INCLUDE_DIRS})

set(obs-ffmpeg_HEADERS
	obs-ffmpeg-formats.h
	obs-ffmpeg-compat.h
	closest-pixel-format.h)

set(obs-ffmpeg_SOURCES
	obs-ffmpeg.c
	obs-ffmpeg-audio-encoders.c
	obs-ffmpeg-nvenc.c
	obs-ffmpeg-output.c
	obs-ffmpeg-mux.c
	obs-ffmpeg-source.c)

if(UNIX AND NOT APPLE)
	list(APPEND obs-ffmpeg_SOURCES
		obs-ffmpeg-vaapi.c)
	LIST(APPEND obs-ffmpeg_PLATFORM_DEPS
		${LIBVA_LBRARIES})
endif()

if(WIN32)
	list(APPEND obs-ffmpeg_SOURCES
		jim-nvenc.c
		jim-nvenc-helpers.c)
	list(APPEND obs-ffmpeg_HEADERS
		jim-nvenc.h)
endif()

add_library(obs-ffmpeg MODULE
	${obs-ffmpeg_HEADERS}
	${obs-ffmpeg_SOURCES})
target_link_libraries(obs-ffmpeg
	libobs
	media-playback
	${obs-ffmpeg_PLATFORM_DEPS}
	${FFMPEG_LIBRARIES})

install_obs_plugin_with_data(obs-ffmpeg data)

add_subdirectory(ffmpeg-mux)
