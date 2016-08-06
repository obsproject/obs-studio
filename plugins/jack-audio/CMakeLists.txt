project(jack-audio)

if(DISABLE_JACK)
	message(STATUS "JACK support disabled")
	return()
endif()

find_package(Jack)
if(NOT JACK_FOUND AND ENABLE_JACK)
	message(FATAL_ERROR "JACK Audio Connection Kit not found but set as enabled")
elseif(NOT JACK_FOUND)
	message(STATUS "JACK Audio Connection Kit not found, disabling JACK plugin")
	return()
endif()

include_directories(
	SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
	${JACK_INCLUDE_DIR}
)
add_definitions(${JACK_DEFINITIONS})

if(MSVC)
	set(jack-audio_PLATFORM_DEPS
		w32-pthreads)
endif()

set(jack-audio_SOURCES
	obs-jack.c
	jack-wrapper.c
	jack-input.c
)

add_library(jack-audio MODULE
	${jack-audio_SOURCES}
)
target_link_libraries(jack-audio
	libobs
	${JACK_LIBRARIES}
	${jack-audio_PLATFORM_DEPS}
)

install_obs_plugin_with_data(jack-audio data)
