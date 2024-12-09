cmake_minimum_required(VERSION 3.28...3.30)

option(ENABLE_FFMPEG_LOGGING "Enables obs-ffmpeg logging" OFF)
option(ENABLE_NEW_MPEGTS_OUTPUT "Use native SRT/RIST mpegts output" ON)

if(OS_LINUX OR OS_WINDOWS)
  option(ENABLE_FFMPEG_NVENC "Enable legacy FFmpeg NVENC encoder" OFF)
endif()

include(cmake/dependencies.cmake)

add_library(obs-ffmpeg MODULE)
add_library(OBS::ffmpeg ALIAS obs-ffmpeg)

add_subdirectory(ffmpeg-mux)

target_sources(
  obs-ffmpeg
  PRIVATE
    $<$<BOOL:${ENABLE_FFMPEG_LOGGING}>:obs-ffmpeg-logging.c>
    $<$<BOOL:${ENABLE_FFMPEG_NVENC}>:obs-ffmpeg-nvenc.c>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:obs-ffmpeg-mpegts.c>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:obs-ffmpeg-rist.h>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:obs-ffmpeg-srt.h>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:obs-ffmpeg-url.h>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:obs-ffmpeg-vaapi.c>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:vaapi-utils.c>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:vaapi-utils.h>
    $<$<PLATFORM_ID:Windows>:texture-amf-opts.hpp>
    $<$<PLATFORM_ID:Windows>:texture-amf.cpp>
    obs-ffmpeg-audio-encoders.c
    obs-ffmpeg-av1.c
    obs-ffmpeg-compat.h
    obs-ffmpeg-formats.h
    obs-ffmpeg-h264.c
    obs-ffmpeg-hls-mux.c
    obs-ffmpeg-mux.c
    obs-ffmpeg-mux.h
    obs-ffmpeg-output.c
    obs-ffmpeg-output.h
    obs-ffmpeg-source.c
    obs-ffmpeg-video-encoders.c
    obs-ffmpeg.c
)

target_compile_options(obs-ffmpeg PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>)
target_compile_definitions(
  obs-ffmpeg
  PRIVATE
    $<$<BOOL:${ENABLE_FFMPEG_LOGGING}>:ENABLE_FFMPEG_LOGGING>
    $<$<BOOL:${ENABLE_FFMPEG_NVENC}>:ENABLE_FFMPEG_NVENC>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:NEW_MPEGTS_OUTPUT>
)

target_link_libraries(
  obs-ffmpeg
  PRIVATE
    OBS::libobs
    OBS::media-playback
    OBS::opts-parser
    FFmpeg::avcodec
    FFmpeg::avfilter
    FFmpeg::avformat
    FFmpeg::avdevice
    FFmpeg::avutil
    FFmpeg::swscale
    FFmpeg::swresample
    $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>
    $<$<PLATFORM_ID:Windows>:AMF::AMF>
    $<$<PLATFORM_ID:Windows>:ws2_32>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libva::va>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libva::drm>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libpci::pci>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:Libdrm::Libdrm>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:Librist::Librist>
    $<$<BOOL:${ENABLE_NEW_MPEGTS_OUTPUT}>:Libsrt::Libsrt>
)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-ffmpeg.rc)
  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg.rc)
endif()

set_target_properties_obs(obs-ffmpeg PROPERTIES FOLDER plugins/obs-ffmpeg PREFIX "")
