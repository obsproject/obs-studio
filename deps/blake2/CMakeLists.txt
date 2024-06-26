cmake_minimum_required(VERSION 3.22...3.25)

add_library(blake2 OBJECT)
add_library(OBS::blake2 ALIAS blake2)

target_sources(
  blake2
  PRIVATE src/blake2-impl.h src/blake2b-ref.c
  PUBLIC src/blake2.h)

target_include_directories(blake2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_target_properties(blake2 PROPERTIES FOLDER deps)

if(OS_WINDOWS)
  add_library(blake2_static OBJECT)
  add_library(OBS::blake2_static ALIAS blake2_static)

  target_sources(
    blake2_static
    PRIVATE src/blake2-impl.h src/blake2b-ref.c
    PUBLIC src/blake2.h)

  target_include_directories(blake2_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
  set_target_properties(blake2_static PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
