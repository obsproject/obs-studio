cmake_minimum_required(VERSION 3.22...3.25)

add_library(caption STATIC EXCLUDE_FROM_ALL )
add_library(OBS::caption ALIAS caption)

target_sources(
  caption
  PRIVATE # cmake-format: sortable
          caption/cea708.h
          caption/eia608.h
          caption/eia608_charmap.h
          caption/mpeg.h
          caption/scc.h
          caption/utf8.h
          src/caption.c
          src/cea708.c
          src/eia608.c
          src/eia608_charmap.c
          src/eia608_from_utf8.c
          src/mpeg.c
          src/scc.c
          src/srt.c
          src/utf8.c
          src/xds.c
  PUBLIC caption/caption.h)

target_include_directories(
  caption
  PRIVATE caption
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(caption PRIVATE __STDC_CONSTANT_MACROS)

# cmake-format: off
target_compile_options(
  caption
  PRIVATE $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-Wno-unused-but-set-parameter>
          $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-Wno-strict-prototypes>
          $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>:-Wno-comma>)
# cmake-format: on

set_target_properties(caption PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE TRUE)
