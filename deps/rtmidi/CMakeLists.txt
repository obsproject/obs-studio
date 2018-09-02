project(rtmidi)

include_directories(
	/
)

if(WIN32)
	set(midi_DEPS
		winmm.lib)
elseif(APPLE)
	find_library(COREMIDI CoreMIDI)
	include_directories(${COREMIDI})
	find_library(COREAUDIO CoreAudio)
	include_directories(${COREAUDIO})
	find_library(COREFOUNDATION CoreFoundation)
	include_directories(${COREFOUNDATION})

	set(midi_DEPS
		${COREMIDI}
		${COREAUDIO}
		${COREFOUNDATION})
elseif(UNIX)
	find_package(ALSA)
	if(NOT ALSA_FOUND)
		message(STATUS "ALSA not found: MIDI disabled")
	else(ALSA_FOUND)
		include_directories(${ALSA_INCLUDE_DIR})

		set(midi_DEPS
			${ALSA_LIBRARY})
	endif()
endif()

set(RTMIDI_SOURCES
	RtMidi.cpp
	rtmidi_c.cpp)

set(RTMIDI_HEADERS
	RtMidi.h
	rtmidi_c.h)

add_library(rtmidi STATIC
        ${RTMIDI_HEADERS}
        ${RTMIDI_SOURCES})

if(WIN32)
	target_compile_definitions(rtmidi PUBLIC __WINDOWS_MM__=1)
elseif(APPLE)
	target_compile_definitions(rtmidi PUBLIC __MACOSX_CORE__=1)
elseif(UNiX)
	if(ALSA_FOUND)
		target_compile_definitions(rtmidi PUBLIC __LINUX_ALSA__=1)
	endif()
endif()

target_link_libraries(rtmidi
	${midi_DEPS})

target_include_directories(rtmidi
	PUBLIC .
)
