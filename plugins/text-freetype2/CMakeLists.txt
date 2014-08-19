project(text-freetype2)

find_package(Freetype QUIET)
if(NOT LIBFREETYPE_FOUND)
	message(STATUS "Freetype library not found, Freetype text plugin disabled")
	return()
endif()

if(APPLE)
	find_package(iconv QUIET)
	if(NOT LIBICONV_FOUND)
		message(STATUS "IConv library not found, Freetype text plugin disabled")
		return()
	endif()

	find_library(COCOA Cocoa)

	set(text-freetype2_PLATFORM_SOURCES
		find-font-cocoa.m
		find-font-iconv.c)

	include_directories(${COCOA}
		${LIBICONV_INCLUDE_DIRS})

	set(text-freetype2_PLATFORM_DEPS
		${COCOA}
		${LIBICONV_LIBRARIES})

	set_source_files_properties(find-font-cocoa.m
		PROPERTIES LANGUAGE C)
elseif(WIN32)
	set(text-freetype2_PLATFORM_SOURCES
		find-font-windows.c)
else()
	message(STATUS "Linux-specific code has yet to be written for the text plugin, just needs load_os_font_list written..  which, er, may or may not be a pain.  (My apologies in advance, please don't strangle me)")
	return()

	find_package(iconv QUIET)
	if(NOT LIBICONV_FOUND)
		message(STATUS "IConv library not found, Freetype text plugin disabled")
		return()
	endif()

	set(text-freetype2_PLATFORM_SOURCES
		find-font-iconv.c)

	include_directories(${LIBICONV_INCLUDE_DIR})
endif()

include_directories(${LIBFREETYPE_INCLUDE_DIRS})
add_definitions(${LIBFREETYPE_DEFINITIONS})

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
	${LIBFREETYPE_LIBRARIES})
if(UNIX AND LIBICONV_FOUND)
	target_link_libraries(text-freetype2 ${LIBICONV_LIBRARIES})
endif()

install_obs_plugin_with_data(text-freetype2 data)
