cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

include(cmake/dependencies.cmake)

add_library(obs-mpegts MODULE)
add_library(OBS::mpegts ALIAS obs-mpegts)

find_package(Librist REQUIRED)
find_package(Libsrt REQUIRED)

target_sources(obs-mpegts PRIVATE obs-mpegts.c obs-mpegts-common.h obs-mpegts-rist.h obs-mpegts-srt.h)
target_compile_options(obs-ffmpeg PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>)
target_link_libraries(
    obs-mpegts
    PRIVATE OBS::libobs
            FFmpeg::avcodec
            FFmpeg::avformat
            FFmpeg::avutil
            $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>
            $<$<PLATFORM_ID:Windows>:ws2_32>
            Librist::Librist
            Libsrt::Libsrt)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-mpegts.rc)
  target_sources(obs-mpegts PRIVATE obs-mpegts.rc)
endif()

# cmake-format: off
set_target_properties_obs(obs-mpegts PROPERTIES FOLDER plugins/obs-mpegts PREFIX "")
# cmake-format: on
