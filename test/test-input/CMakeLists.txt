cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

option(ENABLE_TEST_INPUT "Build test sources" OFF)

if(NOT ENABLE_TEST_INPUT)
  target_disable(test-input)
  return()
endif()

add_library(test-input MODULE)
add_library(OBS::test-input ALIAS test-input)

target_sources(
  test-input
  PRIVATE test-filter.c
          test-input.c
          test-sinewave.c
          sync-async-source.c
          sync-audio-buffering.c
          sync-pair-vid.c
          sync-pair-aud.c
          test-random.c)

target_link_libraries(test-input PRIVATE OBS::libobs)

set_target_properties_obs(test-input PROPERTIES FOLDER "Tests and Examples" PREFIX "")
