cmake_minimum_required (VERSION 2.8.11)
project(rtmidi)

#if (WIN32)
#    add_definitions(-D__WINDOWS_MM__)
#elseif(APPLE)
#    add_definitions(-D__MACOSX_CORE__)
#elseif(UNIX)
#    add_definitions(-D__LINUX_ALSA__)
#endif()

include_directories(
	/
	)

set(RTMIDI_SOURCES
	RtMidi.cpp
	rtmidi_c.cpp)

set(RTMIDI_HEADERS
	RtMidi.h
	rtmidi_c.h)

add_library(rtmidi STATIC
        ${RTMIDI_HEADERS}
        ${RTMIDI_SOURCES})

target_include_directories(rtmidi
	PUBLIC .
)