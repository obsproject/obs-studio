cmake_minimum_required(VERSION 3.22...3.25)

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

add_library(inject-library INTERFACE)
add_library(OBS::inject-library ALIAS inject-library)
target_sources(inject-library INTERFACE inject-library.c inject-library.h)
target_link_libraries(inject-library INTERFACE OBS::obfuscate)
target_include_directories(inject-library INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
