if(NOT ENABLE_WIN_UPDATER)
	return()
endif()

if(DISABLE_UPDATE_MODULE)
	return()
endif()

if(NOT DEFINED STATIC_ZLIB_PATH OR "${STATIC_ZLIB_PATH}" STREQUAL "")
	message(STATUS "STATIC_ZLIB_PATH not set, windows updater disabled")
	return()
endif()

project(updater)

include_directories(${OBS_JANSSON_INCLUDE_DIRS})
include_directories(${LIBLZMA_INCLUDE_DIRS})
include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")
include_directories(${BLAKE2_INCLUDE_DIR})

find_package(ZLIB REQUIRED)

set(updater_HEADERS
	../win-update-helpers.hpp
	resource.h
	updater.hpp
	)
set(updater_SOURCES
	../win-update-helpers.cpp
	updater.cpp
	patch.cpp
	http.cpp
	hash.cpp
	updater.rc
	)

add_definitions(-DNOMINMAX -DUNICODE -D_UNICODE)
if(MSVC)
	add_compile_options("$<$<CONFIG:RelWithDebInfo>:/MT>")
endif()

add_executable(updater WIN32
	${updater_HEADERS}
	${updater_SOURCES}
	)
target_link_libraries(updater
	${OBS_JANSSON_IMPORT}
	${STATIC_ZLIB_PATH}
	lzma
	blake2
	psapi
	comctl32
	shell32
	winhttp
	)
