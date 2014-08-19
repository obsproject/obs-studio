project(mac-avcapture)

find_library(AVFOUNDATION AVFoundation)
find_library(COCOA Cocoa)
find_library(COREFOUNDATION CoreFoundation)
find_library(COREMEDIA CoreMedia)
find_library(COREVIDEO CoreVideo)
find_library(COCOA Cocoa)

include_directories(${AVFOUNDATION}
                    ${COCOA}
                    ${COREFOUNDATION}
                    ${COREMEDIA}
                    ${COREVIDEO}
		    ${COCOA})

set(mac-avcapture_HEADERS
	AVCaptureInputPort+PreMavericksCompat.h)

set(mac-avcapture_SOURCES
	av-capture.m
	plugin-main.c)

set_source_files_properties(av-capture.m
	PROPERTIES LANGUAGE C)
	
add_library(mac-avcapture MODULE
	${mac-avcapture_SOURCES}
	${mac-avcapture_HEADERS})

target_link_libraries(mac-avcapture
	libobs
	${AVFOUNDATION}
	${COCOA}
	${COREFOUNDATION}
	${COREMEDIA}
	${COREVIDEO}
	${COCOA})

install_obs_plugin_with_data(mac-avcapture data)
