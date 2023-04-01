cmake_minimum_required(VERSION 3.16...3.25)

add_library(libff-util INTERFACE EXCLUDE_FROM_ALL)
add_library(OBS::libff-util ALIAS libff-util)

target_sources(libff-util INTERFACE libff/ff-util.c libff/ff-util.h)
target_include_directories(libff-util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(OS_WINDOWS)
  set_property(
    SOURCE libff/ff-util.c
    APPEND
    PROPERTY COMPILE_FLAGS -Dinline=__inline)
endif()
