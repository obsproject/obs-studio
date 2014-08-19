project(obs-ffmpeg)

if(WIN32)
	set(obs-ffmpeg_PLATFORM_DEPS
		w32-pthreads)
endif()

find_package(Libavcodec REQUIRED)
include_directories(${LIBAVCODEC_INCLUDE_DIRS})
add_definitions(${LIBAVCODEC_DEFINITIONS})

find_package(Libavutil REQUIRED)
include_directories(${LIBAVUTIL_INCLUDE_DIRS})
add_definitions(${LIBAVUTIL_DEFINITIONS})

find_package(Libswscale REQUIRED)
include_directories(${LIBSWSCALE_INCLUDE_DIRS})
add_definitions(${LIBSWSCALE_DEFINITIONS})

find_package(Libavformat REQUIRED)
include_directories(${LIBAVFORMAT_INCLUDE_DIRS})
add_definitions(${LIBAVFORMAT_DEFINITIONS})

find_package(Libswresample REQUIRED)
include_directories(${LIBSWRESAMPLE_INCLUDE_DIRS})
add_definitions(${LIBSWRESAMPLE_DEFINITIONS})

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
	${LIBAVCODEC_LIBRARIES}
	${LIBAVUTIL_LIBRARIES}
	${LIBSWSCALE_LIBRARIES}
	${LIBAVFORMAT_LIBRARIES}
	${LIBSWRESAMPLE_LIBRARIES})

install_obs_plugin_with_data(obs-ffmpeg data)
