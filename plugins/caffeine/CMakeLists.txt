project(caffeine)

find_package(libcaffeine REQUIRED)
find_package(Threads REQUIRED)

if(MSVC)
	set(caffeine_PLATFORM_DEPS
		shlwapi.lib
		w32-pthreads)
endif()

set(caffeine_HEADERS
	caffeine-foreground-process.h
	caffeine-settings.h)

set(caffeine_SOURCES
	caffeine-foreground-process.c
	caffeine-module.c
	caffeine-output.c)

add_library(caffeine MODULE
	${caffeine_HEADERS}
	${caffeine_SOURCES})

target_link_libraries(caffeine
	${caffeine_PLATFORM_DEPS}
	${THREADS_LIBRARIES}
	libobs
	libcaffeine)

install_obs_plugin_with_data(caffeine data)

# TODO: There may be a better way to do this, but generator expression didn't
# work with the install_obs_bin helper
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(copy_bin ${LIBCAFFEINE_BINARY_DEBUG})
else()
    set(copy_bin ${LIBCAFFEINE_BINARY_RELWITHDEBINFO})
endif()

install_obs_bin(caffeine "" ${copy_bin})
