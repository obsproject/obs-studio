cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_NVENC "Build NVIDIA Hardware Encoder Plugin" ON)
option(ENABLE_NVENC_FFMPEG_IDS "Register FFmpeg encoder IDs" ON)
mark_as_advanced(ENABLE_NVENC_FFMPEG_IDS)

if(NOT ENABLE_NVENC)
  target_disable_feature(obs-nvenc "NVIDIA Hardware Encoder")
  target_disable(obs-nvenc)
  return()
endif()

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/opts-parser" "${CMAKE_BINARY_DIR}/shared/opts-parser")
endif()

if(OS_LINUX AND NOT TARGET OBS::glad)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/glad" "${CMAKE_BINARY_DIR}/deps/glad")
endif()

find_package(FFnvcodec 12 REQUIRED)

add_library(obs-nvenc MODULE)
add_library(OBS::nvenc ALIAS obs-nvenc)

add_subdirectory(obs-nvenc-test)

target_sources(
  obs-nvenc
  PRIVATE # cmake-format: sortable
    $<$<PLATFORM_ID:Linux>:nvenc-opengl.c>
    $<$<PLATFORM_ID:Windows>:nvenc-d3d11.c>
    cuda-helpers.c
    cuda-helpers.h
    nvenc-compat.c
    nvenc-cuda.c
    nvenc-helpers.c
    nvenc-helpers.h
    nvenc-internal.h
    nvenc-opts-parser.c
    nvenc-properties.c
    nvenc.c
    obs-nvenc.c
    obs-nvenc.h
)

target_link_libraries(
  obs-nvenc
  PRIVATE OBS::libobs OBS::opts-parser FFnvcodec::FFnvcodec $<$<PLATFORM_ID:Linux>:OBS::glad>
)

target_compile_definitions(obs-nvenc PRIVATE $<$<BOOL:${ENABLE_NVENC_FFMPEG_IDS}>:REGISTER_FFMPEG_IDS>)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-nvenc.rc)
  target_sources(obs-nvenc PRIVATE obs-nvenc.rc)
endif()

# cmake-format: off
set_target_properties_obs(obs-nvenc PROPERTIES FOLDER plugins/obs-nvenc PREFIX "")
# cmake-format: on
