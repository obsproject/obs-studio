# TODO: Add posix support
if(NOT WIN32)
	return()
endif()

project(ipc-util)

set(ipc-util_HEADERS
	ipc-util/pipe.h)

if(WIN32)
	set(ipc-util_HEADERS
		${ipc-util_HEADERS}
		ipc-util/pipe-windows.h)
	set(ipc-util_SOURCES
		ipc-util/pipe-windows.c)
else()
	set(ipc-util_HEADERS
		${ipc-util_HEADERS}
		ipc-util/pipe-posix.h)
	set(ipc-util_SOURCES
		ipc-util/pipe-posix.c)
endif()

if(MSVC)
	add_compile_options("$<$<CONFIG:RelWithDebInfo>:/MT>")
endif()

add_library(ipc-util STATIC
	${ipc-util_SOURCES}
	${ipc-util_HEADERS})
target_include_directories(ipc-util
	PUBLIC .)
target_link_libraries(ipc-util)
