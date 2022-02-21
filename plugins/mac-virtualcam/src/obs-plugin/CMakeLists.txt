project(mac-virtualcam)

find_library(APPKIT AppKit)
find_library(COREFOUNDATION CoreFoundation)

include_directories(${APPKIT}
					${COREFOUNDATION}
					../common)

set(mac-virtualcam_HEADERS
	Defines.h
	OBSDALMachServer.h
	../common/MachProtocol.h)

set(mac-virtualcam_SOURCES
	plugin-main.mm
	OBSDALMachServer.mm)

add_library(mac-virtualcam MODULE
	${mac-virtualcam_SOURCES}
	${mac-virtualcam_HEADERS})

target_link_libraries(mac-virtualcam
	libobs
	${APPKIT}
	${COREFOUNDATION})

set_target_properties(mac-virtualcam PROPERTIES
	FOLDER "plugins"
	COMPILE_FLAGS "-std=gnu++14 -stdlib=libc++ -fobjc-arc -fobjc-weak"
)

install_obs_plugin_with_data(mac-virtualcam data)
