project(text-freetype2)
 
find_package(Freetype REQUIRED)
 
include_directories(
	SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
	${LIBFREETYPE_INCLUDE_DIRS})
 
set(text-freetype2_SOURCES
	obs-convenience.c
	text-functionality.c
	text-freetype2.c
	obs-convenience.h
	text-freetype2.h)
 
add_library(text-freetype2 MODULE
	${text-freetype2_SOURCES})
target_link_libraries(text-freetype2
	libobs
	${LIBFREETYPE_LIBRARIES})
 
install_obs_plugin(text-freetype2)
install_obs_plugin_data(text-freetype2 data)
