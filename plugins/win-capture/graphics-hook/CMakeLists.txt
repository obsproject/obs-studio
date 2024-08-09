cmake_minimum_required(VERSION 3.24...3.25)

find_package(Detours REQUIRED)
find_package(Vulkan REQUIRED)

if(NOT TARGET OBS::ipc-util)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/ipc-util" ipc-util)
endif()

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

if(NOT TARGET OBS::d3d8-api)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-d3d8-api" obs-d3d8-api)
endif()

if(NOT TARGET OBS::hook-config)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/obs-hook-config" obs-hook-config)
endif()

add_library(graphics-hook MODULE)
add_library(OBS::graphics-hook ALIAS graphics-hook)

target_sources(
  graphics-hook
  PRIVATE
    d3d10-capture.cpp
    d3d11-capture.cpp
    d3d12-capture.cpp
    d3d8-capture.cpp
    d3d9-capture.cpp
    d3d9-patches.hpp
    dxgi-capture.cpp
    gl-capture.c
    gl-decs.h
    graphics-hook.c
    graphics-hook.h
    graphics-hook.rc
)

target_compile_definitions(graphics-hook PRIVATE COMPILE_D3D12_HOOK)

target_link_libraries(
  graphics-hook
  PRIVATE OBS::d3d8-api OBS::hook-config OBS::ipc-util OBS::obfuscate Detours::Detours dxguid
)

target_link_options(graphics-hook PRIVATE /IGNORE:4099)

if(TARGET Vulkan::Vulkan)
  target_sources(graphics-hook PRIVATE vulkan-capture.c vulkan-capture.h)
  target_link_libraries(graphics-hook PRIVATE Vulkan::Vulkan)
  target_compile_definitions(graphics-hook PRIVATE COMPILE_VULKAN_HOOK)
endif()

if(OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_GENERATOR_PLATFORM)
  if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
    add_custom_command(
      TARGET graphics-hook
      POST_BUILD
      COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t graphics-hook
      COMMENT "Build x86 graphics-hook"
    )
  endif()

  add_dependencies(win-capture graphics-hook)
endif()

if(CMAKE_GENERATOR_PLATFORM STREQUAL x64)
  set(_OUTPUT_NAME graphics-hook64)
else()
  set(_OUTPUT_NAME graphics-hook32)
endif()

set_target_properties_obs(
  graphics-hook PROPERTIES
                FOLDER "plugins/win-capture"
                OUTPUT_NAME ${_OUTPUT_NAME}
                MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)
