project(obs-ffmpeg)

if(WIN32)
	set(obs-ffmpeg_PLATFORM_DEPS
		w32-pthreads)
endif()

find_package(Libavcodec REQUIRED)
include_directories(${Libavcodec_INCLUDE_DIR})
add_definitions(${Libavcodec_DEFINITIONS})

find_package(Libavutil REQUIRED)
include_directories(${Libavutil_INCLUDE_DIR})
add_definitions(${Libavutil_DEFINITIONS})

find_package(Libswscale REQUIRED)
include_directories(${Libswscale_INCLUDE_DIR})
add_definitions(${Libswscale_DEFINITIONS})

find_package(Libavformat REQUIRED)
include_directories(${Libavformat_INCLUDE_DIR})
add_definitions(${Libavformat_DEFINITIONS})

find_package(Libswresample REQUIRED)
include_directories(${Libswresample_INCLUDE_DIR})
add_definitions(${Libswresample_DEFINITIONS})

set(obs-ffmpeg_HEADERS
	obs-ffmpeg-formats.h)
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
	${Libavcodec_LIBRARIES}
	${Libavutil_LIBRARIES}
	${Libswscale_LIBRARIES}
	${Libavformat_LIBRARIES}
	${Libswresample_LIBRARIES})

install_obs_plugin(obs-ffmpeg)

