cmake_minimum_required(VERSION 2.8)
project(libcaption)
add_definitions(-D__STDC_CONSTANT_MACROS)
if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Don't need to prefix local includes with "caption/*"
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/caption)

set(CAPTION_SOURCES
  src/utf8.c
  src/srt.c
  src/scc.c
  src/avc.c
  src/xds.c
  src/cea708.c
  src/caption.c
  src/eia608_charmap.c
  src/eia608.c
)

set(CAPTION_HEADERS
  caption/utf8.h
  caption/sei.h
  caption/scc.h
  caption/avc.c
  caption/cea708.h
  caption/eia608.h
  caption/caption.h
  caption/eia608_charmap.h
)

add_library(caption STATIC ${CAPTION_SOURCES})
