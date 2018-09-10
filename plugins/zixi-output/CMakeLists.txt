project(zixi-output)
include (../../cmake/external/ObsPluginHelpers.cmake)
include_directories(${OBS_JANSSON_INCLUDE_DIRS})

set(zixi-output_SOURCES
	zixi-output.c
	zixi-service.c
	zixi-services-main.c)

set(zixi-output_HEADERS
	include/zixi_feeder_interface.h
	zixi-constants.h
	)

set (ZIXI_INTERN_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/")
if (WIN32)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(ZIXI_INTERN_LIB_NAME feeder_interface-x64)
		set(ZIXI_INTERN_LIB_PATH "${ZIXI_INTERN_LIB_PATH}lib/win/64")
	else(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(ZIXI_INTERN_LIB_NAME feeder_interface-Win32)
		set(ZIXI_INTERN_LIB_PATH lib/win/32)
	endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set (ZIXI_INTERN_LIB_FULL_NAME "${ZIXI_INTERN_LIB_PATH}/${ZIXI_INTERN_LIB_NAME}.dll")
elseif(APPLE)
	set(ZIXI_INTERN_LIB_NAME zixiFeeder)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set (ZIXI_INTERN_LIB_PATH "${ZIXI_INTERN_LIB_PATH}lib/mac/64")
	else (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set (ZIXI_INTERN_LIB_PATH lib/mac/32)
	endif (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set (ZIXI_INTERN_LIB_FULL_NAME "${ZIXI_INTERN_LIB_PATH}/lib${ZIXI_INTERN_LIB_NAME}.dylib")

	message (INFO "MAC ZIXI -> ${ZIXI_INTERN_LIB_FULL_NAME}")
else(UNIX)
	set (ZIXI_INTERN_LIB_NAME zixiFeeder)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (ZIXI_INTERN_LIB_PATH lib/linux/64)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
			set (ZIXI_INTERN_LIB_PATH lib/linux/32)
	endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set (ZIXI_INTERN_LIB_FULL_NAME "${ZIXI_INTERN_LIB_PATH}/lib${ZIXI_INTERN_LIB_NAME}.so")
endif()

find_library(ZIXI_INTERN_LIB 
	NAMES ${ZIXI_INTERN_LIB_NAME}
	PATHS ${ZIXI_INTERN_LIB_PATH})

MESSAGE(STATUS "NAME: ${ZIXI_INTERN_LIB_NAME}")
MESSAGE(STATUS "PATH: ${ZIXI_INTERN_LIB_PATH}")

add_library(zixi-output MODULE
	${zixi-output_SOURCES}
	${zixi-output_HEADERS}
	${zixi-output_config_HEADERS})
	
target_link_libraries(zixi-output
	libobs
	file-updater
	${OBS_JANSSON_IMPORT}
	${ZIXI_INTERN_LIB})

target_include_directories(zixi-output
	PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/plugins/zixi-services>")

if(ZIXI_ADVANCED)
	target_compile_definitions(zixi-output PRIVATE ZIXI_ADVANCED)
endif()
install_obs_plugin (zixi-output)
install_obs_bin (${ZIXI_INTERN_LIB_FULL_NAME} "INSTALL_ONLY" ${ZIXI_INTERN_LIB_FULL_NAME})

