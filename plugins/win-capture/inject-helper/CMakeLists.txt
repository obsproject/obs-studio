cmake_minimum_required(VERSION 3.24...3.25)

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

if(NOT TARGET OBS::inject-library)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-inject-library" obs-inject-library)
endif()

add_executable(inject-helper)

target_sources(inject-helper PRIVATE inject-helper.c)

target_link_libraries(inject-helper PRIVATE OBS::inject-library OBS::obfuscate)

if(OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_GENERATOR_PLATFORM)
  if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
    add_custom_command(
      TARGET inject-helper
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t inject-helper
      COMMENT "Build x86 inject-helper"
    )
  endif()

  add_dependencies(win-capture inject-helper)
endif()

if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
  set(_OUTPUT_NAME inject-helper64)
else()
  set(_OUTPUT_NAME inject-helper32)
endif()

set_target_properties_obs(
  inject-helper PROPERTIES
                FOLDER plugins/win-capture
                OUTPUT_NAME ${_OUTPUT_NAME}
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
