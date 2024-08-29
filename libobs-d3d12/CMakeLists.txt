cmake_minimum_required(VERSION 3.24...3.29)

add_library(libobs-d3d12 MODULE)
add_library(OBS::libobs-d3d12 ALIAS libobs-d3d12)

target_sources(libobs-d3d12 PRIVATE d3d12-subsystem.cpp d3d12-subsystem.hpp)

configure_file(cmake/windows/obs-module.rc.in libobs-d3d12.rc)
target_sources(libobs-d3d12 PRIVATE libobs-d3d12.rc)

target_compile_definitions(
  libobs-d3d12
  PRIVATE
    $<$<BOOL:${GPU_PRIORITY_VAL}>:USE_GPU_PRIORITY>
    "$<IF:$<BOOL:${GPU_PRIORITY_VAL}>,GPU_PRIORITY_VAL=${GPU_PRIORITY_VAL},GPU_PRIORITY_VAL=0>"
)

target_link_libraries(libobs-d3d12 PRIVATE OBS::libobs)

# target_enable_feature(libobs "Direct3D 12 renderer")

set_target_properties_obs(
  libobs-d3d12
  PROPERTIES
  FOLDER
  core
  VERSION
  0
  SOVERSION
  ${OBS_VERSION_MAJOR}
  COMPILE_WARNING_AS_ERROR
  FALSE)
