cmake_minimum_required(VERSION 3.28...3.30)

add_library(libobs-vulkan SHARED)
add_library(OBS::libobs-vulkan ALIAS libobs-vulkan)

# Vulkan 只用于头文件，不在编译时链接
find_package(Vulkan REQUIRED)

# 着色器编译系统 - 可选（如果不可用则禁用编译功能）
find_package(glslang QUIET)
if(glslang_FOUND)
  message(STATUS "glslang found - shader compilation enabled")
  set(ENABLE_VULKAN_SHADER_COMPILER ON)
else()
  message(STATUS "glslang not found - shader compilation disabled")
  set(ENABLE_VULKAN_SHADER_COMPILER OFF)
endif()

if(OS_WINDOWS)
  find_package(D3D12 REQUIRED)
endif()

target_sources(
  libobs-vulkan
  PRIVATE
    $<$<PLATFORM_ID:Windows>:vulkan-windows.c>
    vulkan-api.h
    vulkan-helpers.c
    vulkan-helpers.h
    vulkan-indexbuffer.c
    vulkan-loader.c
    vulkan-loader.h
    vulkan-shader.c
    vulkan-stagesurf.c
    vulkan-subsystem.c
    vulkan-subsystem.h
    vulkan-texture2d.c
    vulkan-texture3d.c
    vulkan-texturecube.c
    vulkan-vertexbuffer.c
    vulkan-zstencil.c
    vulkan-shader-compiler.h
    $<$<BOOL:${ENABLE_VULKAN_SHADER_COMPILER}>:vulkan-shader-compiler.c>
    $<$<BOOL:${ENABLE_VULKAN_SHADER_COMPILER}>:vulkan-shader-compiler-glslang.cpp>
    $<$<BOOL:${ENABLE_VULKAN_SHADER_COMPILER}>:vulkan-shader-manager.h>
    $<$<BOOL:${ENABLE_VULKAN_SHADER_COMPILER}>:vulkan-shader-manager.c>
)

target_compile_options(libobs-vulkan PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-strict-prototypes>)

target_include_directories(
  libobs-vulkan
  PRIVATE
    ${Vulkan_INCLUDE_DIRS}
)

# 不链接 Vulkan 库，改为动态加载
target_link_libraries(
  libobs-vulkan
  PRIVATE
    OBS::libobs
    $<$<PLATFORM_ID:Windows>:d3d12.lib>
    $<$<PLATFORM_ID:Windows>:dxgi.lib>
    $<$<BOOL:${ENABLE_VULKAN_SHADER_COMPILER}>:glslang::glslang>
    $<$<BOOL:${ENABLE_VULKAN_SHADER_COMPILER}>:glslang::SPIRV>
)

# GLSL 着色器编译相关设置
if(ENABLE_VULKAN_SHADER_COMPILER)
  target_compile_definitions(
    libobs-vulkan
    PRIVATE
      VULKAN_SHADER_COMPILER_ENABLED
  )
  
  # 为 C++ 编译器设置标准
  set_source_files_properties(
    vulkan-shader-compiler-glslang.cpp
    PROPERTIES
      LANGUAGE CXX
      CXX_STANDARD 17
  )
endif()

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in libobs-vulkan.rc)
  target_sources(libobs-vulkan PRIVATE libobs-vulkan.rc)
endif()

target_enable_feature(libobs "Vulkan renderer")

set_target_properties_obs(
  libobs-vulkan
  PROPERTIES FOLDER core
             VERSION 0
             PREFIX ""
             SOVERSION "${OBS_VERSION_MAJOR}"
)
