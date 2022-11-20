project(graphics-hook)

find_package(Detours REQUIRED)

if(NOT DEFINED ARCH_ARM64)
  find_package(Vulkan REQUIRED)
endif()

add_library(graphics-hook MODULE)
add_library(OBS::graphics-hook ALIAS graphics-hook)

target_sources(
  graphics-hook
  PRIVATE graphics-hook.c
          graphics-hook.h
          gl-capture.c
          gl-decs.h
          d3d8-capture.cpp
          d3d9-capture.cpp
          d3d9-patches.hpp
          dxgi-capture.cpp
          d3d10-capture.cpp
          d3d11-capture.cpp
          d3d12-capture.cpp
          ../../../libobs/util/windows/obfuscate.c
          ../../../libobs/util/windows/obfuscate.h
          ../graphics-hook-ver.h
          ../graphics-hook-info.h
          ../hook-helpers.h
          graphics-hook.rc)

target_include_directories(graphics-hook PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(graphics-hook PRIVATE OBS::ipc-util Detours::Detours
                                            dxguid)
target_link_options(graphics-hook PRIVATE "LINKER:/IGNORE:4099")

if(MSVC)
  target_compile_options(graphics-hook PRIVATE "$<IF:$<CONFIG:Debug>,/MTd,/MT>")
endif()

set_target_properties(
  graphics-hook
  PROPERTIES FOLDER "plugins/win-capture"
             OUTPUT_NAME
             "graphics-hook$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,64,32>")

target_compile_definitions(graphics-hook PRIVATE COMPILE_D3D12_HOOK)

if(TARGET Vulkan::Vulkan)
  target_sources(graphics-hook PRIVATE vulkan-capture.c vulkan-capture.h)

  target_link_libraries(graphics-hook PRIVATE Vulkan::Vulkan)

  target_compile_definitions(graphics-hook PRIVATE COMPILE_VULKAN_HOOK)

  add_target_resource(
    graphics-hook "${CMAKE_CURRENT_SOURCE_DIR}/obs-vulkan64.json"
    "obs-plugins/win-capture/")
  add_target_resource(
    graphics-hook "${CMAKE_CURRENT_SOURCE_DIR}/obs-vulkan32.json"
    "obs-plugins/win-capture/")
endif()

set(OBS_PLUGIN_DESTINATION "${OBS_DATA_DESTINATION}/obs-plugins/win-capture/")
setup_plugin_target(graphics-hook)

add_dependencies(win-capture graphics-hook)
