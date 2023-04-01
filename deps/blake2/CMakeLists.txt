cmake_minimum_required(VERSION 3.16...3.25)

add_library(blake2 STATIC EXCLUDE_FROM_ALL )
add_library(OBS::blake2 ALIAS blake2)

target_sources(blake2 PRIVATE src/blake2b-ref.c src/blake2.h src/blake2-impl.h)

target_include_directories(blake2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
set_source_files_properties(
  src/blake2b-ref.c PROPERTIES COMPILE_OPTIONS
                               "$<$<C_COMPILER_ID:AppleClang,Clang>:-Wno-error=implicit-int-conversion>")

if(OS_WINDOWS)
  target_compile_options(blake2 PRIVATE $<IF:$<CONFIG:Debug>,/MTd,/MT> /Zl)
  target_compile_definitions(blake2 PRIVATE inline=_inline restrict=__restrict)
endif()

set_target_properties(blake2 PROPERTIES FOLDER deps)
