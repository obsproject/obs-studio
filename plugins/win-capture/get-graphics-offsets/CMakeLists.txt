cmake_minimum_required(VERSION 3.24...3.25)

if(NOT TARGET OBS::d3d8-api)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-d3d8-api" obs-d3d8-api)
endif()

if(NOT TARGET OBS::hook-config)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-hook-config" obs-hook-config)
endif()

add_executable(get-graphics-offsets)
target_sources(
  get-graphics-offsets
  PRIVATE d3d8-offsets.cpp d3d9-offsets.cpp dxgi-offsets.cpp get-graphics-offsets.c get-graphics-offsets.h
)

target_link_libraries(
  get-graphics-offsets
  PRIVATE OBS::hook-config OBS::d3d8-api d3d9.lib dxgi.lib d3d11.lib
)

if(OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_GENERATOR_PLATFORM)
  if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
    add_custom_command(
      TARGET get-graphics-offsets
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t get-graphics-offsets
      COMMENT "Build x86 get-graphics-offsets"
    )
  endif()

  add_dependencies(win-capture get-graphics-offsets)
endif()

if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
  set(_OUTPUT_NAME get-graphics-offsets64)
else()
  set(_OUTPUT_NAME get-graphics-offsets32)
endif()

set_target_properties_obs(
  get-graphics-offsets PROPERTIES
                       FOLDER plugins/win-capture
                       OUTPUT_NAME ${_OUTPUT_NAME}
                       MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
