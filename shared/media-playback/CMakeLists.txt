cmake_minimum_required(VERSION 3.22...3.25)

find_package(FFmpeg REQUIRED avcodec avdevice avutil avformat)

add_library(media-playback INTERFACE)
add_library(OBS::media-playback ALIAS media-playback)

target_sources(
  media-playback
  INTERFACE
    media-playback/cache.c
    media-playback/cache.h
    media-playback/closest-format.h
    media-playback/decode.c
    media-playback/decode.h
    media-playback/media-playback.c
    media-playback/media-playback.h
    media-playback/media.c
    media-playback/media.h
)

target_include_directories(media-playback INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

if(OBS_CMAKE_VERSION VERSION_LESS 3.0)
  target_compile_options(media-playback INTERFACE ${ARCH_SIMD_FLAGS})
  target_compile_definitions(media-playback INTERFACE ${ARCH_SIMD_DEFINES})
endif()

target_link_libraries(media-playback INTERFACE FFmpeg::avcodec FFmpeg::avdevice FFmpeg::avutil FFmpeg::avformat)
