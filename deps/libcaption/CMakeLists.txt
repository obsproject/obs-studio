cmake_minimum_required(VERSION 2.8)
project(libcaption)
set(CMAKE_CXX_FLAGS "-Wall -O3")
add_definitions(-D__STDC_CONSTANT_MACROS)
if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

option(debug "Enable debug symbols." OFF)
if (NOT CMAKE_BUILD_TYPE OR NOT debug)
    set(CMAKE_BUILD_TYPE Release)
else()
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Don't need to prefix local includes with "caption/*"
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/caption)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/eia608.c
  COMMAND re2c -bis -o ${CMAKE_CURRENT_SOURCE_DIR}/src/eia608.c ${CMAKE_CURRENT_SOURCE_DIR}/src/eia608.c.re2c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/eia608.c.re2c)

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

add_library(caption ${CAPTION_SOURCES})

if(CMAKE_VERSION VERSION_EQUAL 2.8.12 OR CMAKE_VERSION VERSION_GREATER 2.8.12)
target_include_directories(caption PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_executable(flv2srt examples/flv2srt.c examples/flv.c)
target_link_libraries(flv2srt caption)
#install (FILES flv2srt DESTINATION bin)

add_executable(ts2srt examples/ts2srt.c examples/ts.c)
target_link_libraries(ts2srt caption)
#install (FILES ts2srt DESTINATION bin)

add_executable(flv+srt examples/flv+srt.c examples/flv.c)
target_link_libraries(flv+srt caption)
#install (FILES flv+srt DESTINATION bin)

add_executable(party examples/party.c examples/flv.c)
target_link_libraries(party caption)
#install (FILES party DESTINATION bin)

add_executable(srtdump examples/srtdump.c )
target_link_libraries(srtdump caption)
#install (FILES srtdump DESTINATION bin)

add_executable(srt2vtt examples/srt2vtt.c)
target_link_libraries(srt2vtt caption)
#install (FILES srt2vtt DESTINATION bin)

add_executable(scc2srt examples/scc2srt.c)
target_link_libraries(scc2srt caption)
#install (FILES scc2srt DESTINATION bin)

#include_directories(/usr/local/include)
#add_executable(rtmpspit examples/rtmpspit.c  examples/flv.c)
#target_link_libraries(rtmpspit caption rtmp)
#install (FILES rtmpspit DESTINATION bin)

# unit-tests
#add_executable(eia608_test unit_tests/eia608_test.c )
#target_link_libraries(eia608_test caption)

#add_executable(test_captions unit_tests/test_sei.c )
#target_link_libraries(test_captions caption)

install (TARGETS caption DESTINATION lib EXPORT caption-targets)
install (FILES ${CAPTION_HEADERS} DESTINATION include/caption)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
  ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
