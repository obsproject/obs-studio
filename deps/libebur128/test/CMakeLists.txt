cmake_minimum_required(VERSION 2.8.9)

set(ENABLE_TESTS OFF CACHE BOOL "Build test binaries, needs libsndfile")

if(ENABLE_TESTS)
  find_package(PkgConfig REQUIRED)
  find_pkg_config(SNDFILE sndfile REQUIRED)

  include_directories(${EBUR128_INCLUDE_DIR})
  include_directories(SYSTEM ${SNDFILE_INCLUDE_DIRS})

  add_executable(r128-test-library tests)
  add_executable(minimal-example minimal-example)
  set_property(TARGET r128-test-library APPEND_STRING PROPERTY
      COMPILE_FLAGS " ${SNDFILE_CFLAGS}")
  set_property(TARGET minimal-example APPEND_STRING PROPERTY
      COMPILE_FLAGS " ${SNDFILE_CFLAGS}")
  target_link_libraries(r128-test-library ebur128 ${SNDFILE_LIBRARIES})
  target_link_libraries(minimal-example ebur128 ${SNDFILE_LIBRARIES})
endif()
