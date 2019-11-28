project(libobs-winrt)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

add_definitions(-DLIBOBS_EXPORTS)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(libobs-winrt_SOURCES
	winrt-capture.cpp)

set(libobs-winrt_HEADERS
	winrt-capture.h)

add_library(libobs-winrt MODULE
	${libobs-winrt_SOURCES}
	${libobs-winrt_HEADERS})
set_target_properties(libobs-winrt
	PROPERTIES
		OUTPUT_NAME libobs-winrt
		PREFIX "")
target_link_libraries(libobs-winrt
	libobs
	windowsapp)

install_obs_core(libobs-winrt)
