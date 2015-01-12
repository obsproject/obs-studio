project(win-test)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

set(win-text_SOURCES
	test.cpp)

add_executable(win-test WIN32
	${win-text_SOURCES})
target_link_libraries(win-test
	libobs)
define_graphic_modules(win-test)
