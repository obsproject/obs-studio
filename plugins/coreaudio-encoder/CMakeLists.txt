project(coreaudio-encoder)

set(coreaudio-encoder_SOURCES
	encoder.cpp)

if (WIN32)
	set(coreaudio-encoder_HEADERS windows-imports.h)
	set(coreaudio-encoder_LIBS )

	if (MINGW)
		set_source_files_properties(${coreaudio-encoder_SOURCES}
			PROPERTIES COMPILE_FLAGS "-Wno-multichar")
	endif()
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
