cmake_minimum_required(VERSION 3.24...3.25)

find_package(Detours REQUIRED)
find_package(Vulkan REQUIRED)

if(NOT TARGET OBS::ipc-util)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/ipc-util" "${CMAKE_BINARY_DIR}/deps/ipc-util")
endif()

if(NOT TARGET OBS::obfuscate)
  add_subdirectory("${CMAKE_SOURCE_DIR}/libobs" "${CMAKE_BINARY_DIR}/libobs")
endif()

add_library(_graphics-hook INTERFACE)

target_sources(
  _graphics-hook
  INTERFACE # cmake-format: sortable
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
            graphics-hook.rc)

target_compile_definitions(_graphics-hook INTERFACE COMPILE_D3D12_HOOK)

target_link_libraries(_graphics-hook INTERFACE OBS::d3d8-api OBS::hook-config OBS::ipc-util OBS::obfuscate
                                               Detours::Detours dxguid)

target_link_options(_graphics-hook INTERFACE /IGNORE:4099)

if(TARGET Vulkan::Vulkan)
  target_sources(_graphics-hook INTERFACE vulkan-capture.c vulkan-capture.h)
  target_link_libraries(_graphics-hook INTERFACE Vulkan::Vulkan)
  target_compile_definitions(_graphics-hook INTERFACE COMPILE_VULKAN_HOOK)
endif()

legacy_check()

add_library(graphics-hook MODULE)
add_library(OBS::graphics-hook ALIAS graphics-hook)

target_link_libraries(graphics-hook PRIVATE _graphics-hook)

include(cmake/32bit.cmake)

# cmake-format: off
set_target_properties_obs(graphics-hook PROPERTIES FOLDER "plugins/win-capture" OUTPUT_NAME graphics-hook64 MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# cmake-format: on

add_dependencies(win-capture graphics-hook)
