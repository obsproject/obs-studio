cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_JACK "Build OBS with JACK support" OFF)
if(NOT ENABLE_JACK)
  target_disable(linux-jack)
  return()
endif()

find_package(Jack REQUIRED)

add_library(linux-jack MODULE)
add_library(OBS::jack ALIAS linux-jack)

target_sources(linux-jack PRIVATE # cmake-format: sortable
                                  jack-input.c jack-wrapper.c linux-jack.c)

target_compile_options(linux-jack PRIVATE $<$<COMPILE_LANG_AND_ID:C,Clang>:-Wno-error=sign-compare>)

target_link_libraries(linux-jack PRIVATE OBS::libobs Jack::Jack)

set_target_properties_obs(linux-jack PROPERTIES FOLDER plugins PREFIX "" PROJECT_LABEL "JACK Audio")
