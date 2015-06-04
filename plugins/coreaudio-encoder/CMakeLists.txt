project(coreaudio-encoder)

find_library(COREFOUNDATION CoreFoundation)
find_library(COREAUDIO CoreAudio)
find_library(AUDIOTOOLBOX AudioToolbox)

include_directories(${COREFOUNDATION}
	${COREAUDIO}
	${AUDIOTOOLBOX})

set(coreaudio-encoder_SOURCES
	encoder.c)

add_library(coreaudio-encoder MODULE
	${coreaudio-encoder_SOURCES}
	${coreaudio-encoder_HEADERS})

target_link_libraries(coreaudio-encoder
	libobs
	${COREFOUNDATION}
	${COREAUDIO}
	${AUDIOTOOLBOX})

install_obs_plugin_with_data(coreaudio-encoder data)
