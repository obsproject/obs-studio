project(osx-text)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

find_library(COCOA Cocoa)
include_directories(${COCOA})

add_definitions(-fobjc-arc)

set(osx-test_SOURCES
	test.mm)

add_executable(osx_test
	${osx-test_SOURCES})
target_link_libraries(osx_test
	libobs
	${COCOA})
define_graphic_modules(osx_test)
