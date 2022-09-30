project(opts-parser)

add_library(opts-parser INTERFACE)
add_library(OBS::opts-parser ALIAS opts-parser)

target_sources(opts-parser INTERFACE opts-parser.c opts-parser.h)

target_link_libraries(opts-parser INTERFACE OBS::libobs)

target_include_directories(opts-parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
