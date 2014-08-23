project(text-freetype2)

find_package(Freetype QUIET)
if(NOT FREETYPE_FOUND)
	message(STATUS "Freetype library not found, Freetype text plugin disabled")
	return()
endif()

if(APPLE)
	find_package(Iconv QUIET)
	if(NOT ICONV_FOUND)
		message(STATUS "Iconv library not found, Freetype text plugin disabled")
		return()
	endif()

	find_library(COCOA Cocoa)

	set(text-freetype2_PLATFORM_SOURCES
		find-font-cocoa.m
		find-font-iconv.c)

	include_directories(${COCOA}
		${ICONV_INCLUDE_DIRS})

	set(text-freetype2_PLATFORM_DEPS
		${COCOA}
		${ICONV_LIBRARIES})

	set_source_files_properties(find-font-cocoa.m
		PROPERTIES LANGUAGE C)
elseif(WIN32)
	set(text-freetype2_PLATFORM_SOURCES
		find-font-windows.c)
else()
	include(CheckFunctionExists)
	check_function_exists(iconv SYSTEM_ICONV_FOUND)
	find_package(Iconv QUIET)

	if(NOT ICONV_FOUND AND NOT SYSTEM_ICONV_FOUND)
		message(STATUS "Iconv library not found, Freetype text plugin disabled")
		return()
	endif()

	find_package(Fontconfig QUIET)
	if(NOT FONTCONFIG_FOUND)
		message(STATUS "fontconfig not found, Freetype text plugin disabled")
		return()
	endif()

	set(text-freetype2_PLATFORM_SOURCES
		find-font-linux.c
		find-font-iconv.c)

	include_directories(${ICONV_INCLUDE_DIR})
	include_directories(${FONTCONFIG_INCLUDE_DIRS})
endif()

include_directories(${FREETYPE_INCLUDE_DIRS})

set(text-freetype2_SOURCES
	find-font.h
	find-font.c
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

if(UNIX AND ICONV_FOUND)
	target_link_libraries(text-freetype2 ${ICONV_LIBRARIES})
endif()
if(UNIX AND FONTCONFIG_FOUND)
	target_link_libraries(text-freetype2 ${FONTCONFIG_LIBRARIES})
endif()

install_obs_plugin_with_data(text-freetype2 data)
