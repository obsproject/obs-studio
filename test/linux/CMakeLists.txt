project(linux-test)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

find_package(Qt5X11Extras REQUIRED)

set(linux-text_SOURCES
	test.cpp)

add_executable(linux-test
	${linux-text_SOURCES})
target_link_libraries(linux-test
	libobs
	Qt5::Widgets
	Qt5::X11Extras)
define_graphic_modules(linux-test)
