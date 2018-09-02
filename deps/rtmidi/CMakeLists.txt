cmake_minimum_required (VERSION 2.8.11)
project(rtmidi)

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