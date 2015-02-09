project(test-input)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

if(MSVC)
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

install_obs_plugin_with_data(test-input data)
