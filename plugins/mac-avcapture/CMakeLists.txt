project(mac-avcapture)

find_library(AVFOUNDATION AVFoundation)
find_library(COREFOUNDATION CoreFoundation)
find_library(COREMEDIA CoreMedia)
find_library(COREVIDEO CoreVideo)


include_directories(${AVFOUNDATION}
                    ${COREFOUNDATION}
                    ${COREMEDIA}
                    ${COREVIDEO})

set(mac-avcapture_HEADERS
	)

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
	${COREFOUNDATION}
	${COREMEDIA}
	${COREVIDEO})

install_obs_plugin(mac-avcapture)
