project(opts-parser)

set(opts-parser_SOURCES
	opts-parser.c)
set(opts-parser_HEADERS
	opts-parser.h)

add_library(opts-parser STATIC
	${opts-parser_SOURCES}
	${opts-parser_HEADERS})
target_include_directories(opts-parser
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(opts-parser
	libobs)
set_target_properties(opts-parser PROPERTIES
	FOLDER "deps"
	POSITION_INDEPENDENT_CODE ON)
