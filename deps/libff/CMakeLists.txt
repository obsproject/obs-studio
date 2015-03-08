cmake_minimum_required (VERSION 2.8.11)
project (libff)

find_package(FFMpeg REQUIRED
	COMPONENTS avcodec avfilter avdevice avutil swscale avformat swresample)

include_directories(${FFMPEG_INCLUDE_DIRS})

if(WIN32)
	include_directories(../w32-pthreads)
	add_definitions(-Dinline=__inline)
endif(WIN32)

set(libff_HEADERS
	libff/ff-callbacks.h
	libff/ff-circular-queue.h
	libff/ff-clock.h
	libff/ff-frame.h
	libff/ff-packet-queue.h
	libff/ff-timer.h
	#
	libff/ff-demuxer.h
	#
	libff/ff-decoder.h)

set(libff_SOURCES
	libff/ff-callbacks.c
	libff/ff-circular-queue.c
	libff/ff-clock.c
	libff/ff-packet-queue.c
	libff/ff-timer.c
	#
	libff/ff-demuxer.c
	#
	libff/ff-decoder.c
	libff/ff-audio-decoder.c
	libff/ff-video-decoder.c)

add_library (libff STATIC
	${libff_HEADERS}
	${libff_SOURCES})

target_include_directories(libff
	PUBLIC .)

if(NOT MSVC)
        if(NOT MINGW)
                target_compile_options(libff PRIVATE -fPIC)
        endif()
endif()

target_link_libraries (libff)
