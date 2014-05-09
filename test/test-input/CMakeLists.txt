project(test-input)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

if(APPLE)
	set(test-input_PLATFORM_SOURCES
		test-desktop.m)

	set_source_files_properties(test-desktop.m
		PROPERTIES LANGUAGE C
			COMPILE_FLAGS "-fobjc-arc")

	find_library(IOSURF IOSurface)
	include_directories(${IOSURF})
	mark_as_advanced(${IOSURF})

	find_library(COCOA Cocoa)
	include_directories(${COCOA})
	mark_as_advanced(${COCOA})

	set(test-input_PLATFORM_DEPS
		${IOSURF}
		${COCOA})
elseif(WIN32)
	set(test-input_PLATFORM_DEPS
		w32-pthreads)
endif()

set(test-input_SOURCES
	${test-input_PLATFORM_SOURCES}
	test-filter.c
	test-input.c
	test-sinewave.c
	test-random.c)

add_library(test-input MODULE
	${test-input_SOURCES})

target_link_libraries(test-input
	${test-input_PLATFORM_DEPS}
	libobs)

install_obs_plugin_data(test-input ../../build/data/obs-plugins/test-input)
