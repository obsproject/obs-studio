project(mac-capture)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(COREFOUNDATION CoreFoundation)

include_directories(${COREAUDIO}
                    ${AUDIOUNIT}
                    ${COREFOUNDATION})

set(mac-capture_SOURCES
	plugin-main.c
	mac-audio.c)
	
add_library(mac-capture MODULE
	${mac-capture_SOURCES})
target_link_libraries(mac-capture
	libobs
	${COREAUDIO}
	${AUDIOUNIT}
	${COREFOUNDATION})

install_obs_plugin(mac-capture)
