project(osx-text)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

find_library(COCOA Cocoa)
include_directories(${COCOA})

add_definitions(-fobjc-arc)

set(osx-text_SOURCES
	test.mm)

add_executable(test
	${osx-text_SOURCES})
target_link_libraries(test
	libobs
	${COCOA})
