project(test-input)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(test-input_SOURCES
	test-desktop.m
	test-filter.c
	test-input.c
	test-sinewave.c
	test-random.c)

set(test-input_HEADERS
	test-filter.h
	test-input-exports.h
	test-random.h
	test-sinewave.h)

set_source_files_properties(test-desktop.m
	PROPERTIES LANGUAGE C
		COMPILE_FLAGS "-fobjc-arc")

add_library(test-input MODULE
	${test-input_SOURCES}
	${test-input_HEADERS})

target_link_libraries(test-input
	libobs
	${IOSURF})

install_obs_plugin_data(test-input ../../build/data/obs-plugins/test-input)
