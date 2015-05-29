project(ffmpeg-mux)

find_package(FFMpeg REQUIRED
	COMPONENTS avcodec avutil avformat)
include_directories(${FFMPEG_INCLUDE_DIRS})

set(ffmpeg-mux_SOURCES
	ffmpeg-mux.c)

set(ffmpeg-mux_HEADERS
	ffmpeg-mux.h)

add_executable(ffmpeg-mux
	${ffmpeg-mux_SOURCES}
	${ffmpeg-mux_HEADERS})

target_link_libraries(ffmpeg-mux
	${FFMPEG_LIBRARIES})

if(WIN32)
	set_target_properties(ffmpeg-mux
		PROPERTIES
			OUTPUT_NAME "ffmpeg-mux${_output_suffix}")
endif()

install_obs_datatarget(ffmpeg-mux "obs-plugins/obs-ffmpeg")
