cmake_minimum_required(VERSION 3.16...3.25)

add_library(caption STATIC EXCLUDE_FROM_ALL )
add_library(OBS::caption ALIAS caption)

target_sources(
  caption
  PRIVATE src/caption.c
          src/cea708.c
          src/eia608.c
          src/eia608_charmap.c
          src/eia608_from_utf8.c
          src/mpeg.c
          src/srt.c
          src/scc.c
          src/utf8.c
          src/xds.c
          caption/eia608.h
          caption/eia608_charmap.h
          caption/cea708.h
          caption/mpeg.h
          caption/scc.h
          caption/utf8.h
  PUBLIC caption/caption.h)

target_compile_definitions(caption PRIVATE __STDC_CONSTANT_MACROS $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

target_compile_options(
  caption
  PRIVATE "$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-unused-but-set-parameter;-Wno-comma;-Wno-strict-prototypes>")

target_include_directories(
  caption
  PRIVATE caption
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(caption PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE ON)
