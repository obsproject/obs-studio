project(obs-outputs)

if(WIN32)
	set(obs-outputs_PLATFORM_DEPS
		w32-pthreads
		ws2_32.lib
		winmm.lib)
endif()

set(obs-outputs_librtmp_HEADERS
	librtmp/amf.h
	librtmp/bytes.h
	librtmp/cencode.h
	librtmp/dh.h
	librtmp/dhgroups.h
	librtmp/handshake.h
	librtmp/http.h
	librtmp/log.h
	librtmp/md5.h
	librtmp/rtmp.h
	librtmp/rtmp_sys.h)
set(obs-outputs_librtmp_SOURCES
	librtmp/amf.c
	librtmp/cencode.c
	librtmp/hashswf.c
	librtmp/log.c
	librtmp/md5.c
	librtmp/parseurl.c
	librtmp/rtmp.c)

if(NOT WIN32)
	set_source_files_properties(${obs-outputs_librtmp_SOURCES} PROPERTIES
		COMPILE_FLAGS "-fvisibility=hidden")
endif()

set(obs-outputs_HEADERS
	obs-output-ver.h
	rtmp-helpers.h
	flv-mux.h
	flv-output.h
	librtmp)
set(obs-outputs_SOURCES
	obs-outputs.c
	rtmp-stream.c
	flv-output.c
	flv-mux.c)
	
add_library(obs-outputs MODULE
	${obs-outputs_SOURCES}
	${obs-outputs_HEADER}
	${obs-outputs_librtmp_SOURCES}
	${obs-outputs_librtmp_HEADERS})
target_link_libraries(obs-outputs
	libobs
	${obs-outputs_PLATFORM_DEPS})

install_obs_plugin_with_data(obs-outputs data)
