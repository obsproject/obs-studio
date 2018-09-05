project(rtmp-services)

include_directories(${OBS_JANSSON_INCLUDE_DIRS})

set(rtmp-services_SOURCES
	twitch.c
	rtmp-common.c
	rtmp-custom.c
	rtmp-services-main.c)

set(rtmp-services_HEADERS
	twitch.h
	rtmp-format-ver.h)

set(RTMP_SERVICES_URL
	"https://obsproject.com/obs2_update/rtmp-services"
	CACHE STRING "Default services package URL")
option(CHECK_FOR_SERVICE_UPDATES "Checks for service updates" OFF)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/lookup-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/rtmp-services/lookup-config.h")

set(rtmp-services_config_HEADERS
	"${CMAKE_BINARY_DIR}/plugins/rtmp-services/lookup-config.h")

if (BUILD_ZIXI_OUTPUT)
	set(rtmp-services_SOURCES
		${rtmp-services_SOURCES}
		rtmp-to-zixi.c)
	set(rtmp-services_HEADERS
		${rtmp-services_HEADERS}
		rtmp-to-zixi.h)
		
	set (rtmp-services_INCLUDES
		"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/plugins/rtmp-services>"
		"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/plugins/zixi>")
else()
	set (rtmp-services_INCLUDES
		"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/plugins/rtmp-services>")
endif()	

add_library(rtmp-services MODULE
	${rtmp-services_SOURCES}
	${rtmp-services_HEADERS}
	${rtmp-services_config_HEADERS})
target_link_libraries(rtmp-services
	libobs
	file-updater
	${OBS_JANSSON_IMPORT})
	
if (BUILD_ZIXI_OUTPUT)
	target_compile_definitions(rtmp-services PUBLIC ENABLE_ZIXI)
endif()

target_include_directories(rtmp-services
	PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/plugins/rtmp-services>")

install_obs_plugin_with_data(rtmp-services data)
