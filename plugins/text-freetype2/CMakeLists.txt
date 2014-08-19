project(text-freetype2)

find_package(Freetype QUIET)
if(NOT FREETYPE_FOUND)
	message(STATUS "Freetype library not found, Freetype text plugin disabled")
	return()
endif()

if(WIN32)
	set(text-freetype2_PLATFORM_SOURCES
		find-font-windows.c)
else()
	find_package(Fontconfig QUIET)
	if(NOT FONTCONFIG_FOUND)
		message(STATUS "fontconfig not found, Freetype text plugin disabled")
		return()
	endif()

	set(text-freetype2_PLATFORM_SOURCES
		find-font-unix.c)

	include_directories(${FONTCONFIG_INCLUDE_DIRS})
endif()

include_directories(${FREETYPE_INCLUDE_DIRS})

set(text-freetype2_SOURCES
	find-font.h
	obs-convenience.c
	text-functionality.c
	text-freetype2.c
	obs-convenience.h
	text-freetype2.h)

add_library(text-freetype2 MODULE
	${text-freetype2_PLATFORM_SOURCES}
	${text-freetype2_SOURCES})
target_link_libraries(text-freetype2
	libobs
	${text-freetype2_PLATFORM_DEPS}
	${FREETYPE_LIBRARIES})

if(NOT WIN32)
	if(FONTCONFIG_FOUND)
		target_link_libraries(text-freetype2 ${FONTCONFIG_LIBRARIES})
	endif()
endif()

install_obs_plugin_with_data(text-freetype2 data)
