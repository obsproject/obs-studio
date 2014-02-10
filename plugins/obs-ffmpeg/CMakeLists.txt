project(obs-ffmpeg)

find_package(Libavcodec REQUIRED)
include_directories(${Libavcodec_INCLUDE_DIR})
add_definitions(${Libavcodec_DEFINITIONS})

find_package(Libswscale REQUIRED)
include_directories(${Libswscale_INCLUDE_DIR})
add_definitions(${Libswscale_DEFINITIONS})

find_package(Libavformat REQUIRED)
include_directories(${Libavformat_INCLUDE_DIR})
add_definitions(${Libavformat_DEFINITIONS})

find_package(Libswresample REQUIRED)
include_directories(${Libswresample_INCLUDE_DIR})
add_definitions(${Libswresample_DEFINITIONS})

set(obs-ffmpeg_SOURCES
	obs-ffmpeg.c
	obs-ffmpeg-output.c)

set(obs-ffmpeg_HEADERS
	obs-ffmpeg-output.h)
	
add_library(obs-ffmpeg MODULE
	${obs-ffmpeg_SOURCES}
	${obs-ffmpeg_HEADERS})
target_link_libraries(obs-ffmpeg
	libobs
	${Libavcodec_LIBRARIES}
	${Libswscale_LIBRARIES}
	${Libavformat_LIBRARIES}
	${Libswresample_LIBRARIES})

install_obs_plugin(obs-ffmpeg)

obs_fixup_install_target(obs-ffmpeg PATH ${Libavcodec_LIBRARIES})
obs_fixup_install_target(obs-ffmpeg PATH ${Libswscale_LIBRARIES})
obs_fixup_install_target(obs-ffmpeg PATH ${Libavformat_LIBRARIES})
obs_fixup_install_target(obs-ffmpeg PATH ${Libswresample_LIBRARIES})
