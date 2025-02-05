cmake_minimum_required(VERSION 3.28...3.30)

add_library(libobs-d3d12 MODULE)
add_library(OBS::libobs-d3d12 ALIAS libobs-d3d12)

target_sources(libobs-d3d12 PRIVATE 
    d3d12-indexbuffer.cpp
    d3d12-samplerstate.cpp
    d3d12-shader.cpp
    d3d12-shaderprocessor.cpp
    d3d12-shaderprocessor.hpp
    d3d12-stagesurf.cpp
    d3d12-subsystem.cpp
    d3d12-subsystem.hpp
    d3d12-texture2d.cpp
    d3d12-texture3d.cpp
    d3d12-vertexbuffer.cpp
    d3d12-zstencilbuffer.cpp)

configure_file(cmake/windows/obs-module.rc.in libobs-d3d12.rc)
target_sources(libobs-d3d12 PRIVATE libobs-d3d12.rc)

target_compile_definitions(
  libobs-d3d12
  PRIVATE
    $<$<BOOL:${GPU_PRIORITY_VAL}>:USE_GPU_PRIORITY>
    "$<IF:$<BOOL:${GPU_PRIORITY_VAL}>,GPU_PRIORITY_VAL=${GPU_PRIORITY_VAL},GPU_PRIORITY_VAL=0>"
)

target_link_libraries(libobs-d3d12 PRIVATE OBS::libobs d3d12 d3dcompiler dxgi shcore)

target_enable_feature(libobs "Direct3D 12 renderer")

set_target_properties_obs(
  libobs-d3d12
  PROPERTIES FOLDER core
             VERSION 0
             SOVERSION ${OBS_VERSION_MAJOR}
             COMPILE_WARNING_AS_ERROR FALSE
)
