
project(watcher)


find_package(FFmpeg REQUIRED
	COMPONENTS avcodec avfilter avdevice avutil swscale avformat swresample)
include_directories(${FFMPEG_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})



set(watcher_SOURCES
      watcher.c
      watcher-source.c)

add_library(watcher MODULE
      ${watcher_SOURCES})
target_link_libraries(watcher
      libobs
      media-playback
      ${obs-ffmpeg_PLATFORM_DEPS}
	${FFMPEG_LIBRARIES}
      )

install_obs_plugin_with_data(watcher data)