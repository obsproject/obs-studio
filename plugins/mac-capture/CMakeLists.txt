project(mac-capture)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(COREFOUNDATION CoreFoundation)
find_library(IOSURF IOSurface)
find_library(COCOA Cocoa)

include_directories(${COREAUDIO}
                    ${AUDIOUNIT}
                    ${COREFOUNDATION}
		    ${IOSURF}
		    ${COCOA})

set(mac-capture_HEADERS
	audio-device-enum.h
	mac-helpers.h)

set(mac-capture_SOURCES
	plugin-main.c
	audio-device-enum.c
	mac-audio.c
	mac-screen-capture.m)
	
set_source_files_properties(mac-screen-capture.m
	PROPERTIES LANGUAGE C
		COMPILE_FLAGS "-fobjc-arc")

add_library(mac-capture MODULE
	${mac-capture_SOURCES}
	${mac-capture_HEADERS})
target_link_libraries(mac-capture
	libobs
	${COREAUDIO}
	${AUDIOUNIT}
	${COREFOUNDATION}
	${IOSURF}
	${COCOA})

install_obs_plugin(mac-capture)
install_obs_plugin_data(mac-capture ../../build/data/obs-plugins/mac-capture)
