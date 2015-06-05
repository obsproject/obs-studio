project(coreaudio-encoder)

set(coreaudio-encoder_SOURCES
	encoder.c)

if (WIN32)
	set(coreaudio-encoder_HEADERS windows-imports.h)
	set(coreaudio-encoder_LIBS )
else()
	find_library(COREFOUNDATION CoreFoundation)
	find_library(COREAUDIO CoreAudio)
	find_library(AUDIOTOOLBOX AudioToolbox)
	set(coreaudio-encoder_HEADERS )
	set(coreaudio-encoder_LIBS
		${COREFOUNDATION}
		${COREAUDIO}
		${AUDIOTOOLBOX})

	include_directories(${COREFOUNDATION}
		${COREAUDIO}
		${AUDIOTOOLBOX})
endif()

add_library(coreaudio-encoder MODULE
	${coreaudio-encoder_SOURCES}
	${coreaudio-encoder_HEADERS})

target_link_libraries(coreaudio-encoder
	libobs
	${coreaudio-encoder_LIBS})

install_obs_plugin_with_data(coreaudio-encoder data)
