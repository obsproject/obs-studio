cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

add_library(libobs-d3d11 MODULE)
add_library(OBS::libobs-d3d11 ALIAS libobs-d3d11)

target_sources(
  libobs-d3d11
  PRIVATE # cmake-format: sortable
          d3d11-duplicator.cpp
          d3d11-indexbuffer.cpp
          d3d11-rebuild.cpp
          d3d11-samplerstate.cpp
          d3d11-shader.cpp
          d3d11-shaderprocessor.cpp
          d3d11-shaderprocessor.hpp
          d3d11-stagesurf.cpp
          d3d11-subsystem.cpp
          d3d11-subsystem.hpp
          d3d11-texture2d.cpp
          d3d11-texture3d.cpp
          d3d11-vertexbuffer.cpp
          d3d11-zstencilbuffer.cpp)

configure_file(cmake/windows/obs-module.rc.in libobs-d3d11.rc)
target_sources(libobs-d3d11 PRIVATE libobs-d3d11.rc)
target_compile_options(libobs-d3d11 PRIVATE /wd4595)
target_compile_definitions(
  libobs-d3d11 PRIVATE "$<$<BOOL:${GPU_PRIORITY_VAL}>:USE_GPU_PRIORITY>"
                       "$<IF:$<BOOL:${GPU_PRIORITY_VAL}>,GPU_PRIORITY_VAL=${GPU_PRIORITY_VAL},GPU_PRIORITY_VAL=0>")

target_link_libraries(libobs-d3d11 PRIVATE OBS::libobs d3d9 d3d11 d3dcompiler dxgi shcore)

target_enable_feature(libobs "Direct3D 11 renderer")

# cmake-format: off
set_target_properties_obs(
  libobs-d3d11
  PROPERTIES FOLDER core
             VERSION 0
             SOVERSION ${OBS_VERSION_MAJOR})
# cmake-format: on
