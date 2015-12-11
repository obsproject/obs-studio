project(input-uiohook)

# libuiohook

if(APPLE)
	find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
	if(APPLICATIONSERVICES_LIBRARY)
		message(STATUS "Found ApplicationServices: ${APPLICATIONSERVICES_LIBRARY}")
	else()
		message(FATAL_ERROR "ApplicationServices not found")
	endif()
	find_library(COCOA_LIBRARY Cocoa)
	if(COCOA_LIBRARY)
		message(STATUS "Found Cocoa: ${COCOA_LIBRARY}")
	else()
		message(FATAL_ERROR "Foundation not found")
	endif()
	find_library(IOKIT_LIBRARY IOKit)
	if(IOKIT_LIBRARY)
		message(STATUS "Found IOKit: ${IOKIT_LIBRARY}")
	else()
		message(FATAL_ERROR "IOKit not found")
	endif()
endif()

include_directories(libuiohook/include)
include_directories(libuiohook/src)

set(ARCHDIR "")
if(APPLE)
	set(ARCHDIR "darwin")
elseif(WIN32 OR WIN64)
	set(ARCHDIR "windows")
elseif(UNIX)
	set(ARCHDIR "x11")
else()
	message(FATAL_ERROR "Architecture not supported")
endif()

include_directories(src/${ARCHDIR})

file(GLOB LIBUIOHOOK_SOURCES 
    libuiohook/src/logger.c 
    libuiohook/src/${ARCHDIR}/*.c)
file(GLOB LIBUIOHOOK_HEADERS 
    libuiohook/src/*.h 
    libuiohook/src/${ARCHDIR}/*.h 
    libuiohook/include/uiohook.h)
add_library(libuiohook 
    ${LIBUIOHOOK_SOURCES} 
    ${LIBUIOHOOK_HEADERS})

if(APPLE)
    target_link_libraries(libuiohook 
        ${APPLICATIONSERVICES_LIBRARY} 
        ${COCOA_LIBRARY} 
        ${IOKIT_LIBRARY})
endif()

# input-uiohook

if(WIN32)
	set(input-uiohook_PLATFORM_SOURCES
		platform-windows.c
)
elseif(APPLE)
	set(input-uiohook_PLATFORM_SOURCES
		platform-osx.m)

	find_package(AppKit REQUIRED)
	set(input-uiohook_PLATFORM_LIBRARIES ${APPKIT_LIBRARIES})

	add_definitions(-fobjc-arc)

elseif(UNIX)
	set(input-uiohook_PLATFORM_SOURCES
		platform-x11.c)

	find_package(XCB COMPONENTS XCB REQUIRED RANDR REQUIRED XINERAMA REQUIRED)
	
	include_directories(
		${XCB_INCLUDE_DIRS}
		${X11_XCB_INCLUDE_DIRS})
		
	add_definitions(
		${XCB_DEFINITIONS}
		${X11_XCB_DEFINITIONS})

	set(input-uiohook_PLATFORM_LIBRARIES
		${XCB_LIBRARIES}
		${X11_XCB_LIBRARIES}
                )
endif()


set(input-uiohook_SOURCES
	${input-uiohook_PLATFORM_SOURCES}
        platform.h
        libuiohook-async-logger.h
        libuiohook-async-logger.c
	input-uiohook.c)

add_library(input-uiohook MODULE
	${input-uiohook_SOURCES})
target_link_libraries(input-uiohook
	${input-uiohook_PLATFORM_LIBRARIES}
	libobs
	libuiohook)

install_obs_plugin_with_data(input-uiohook data)
