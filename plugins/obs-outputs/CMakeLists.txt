cmake_minimum_required(VERSION 3.28...3.30)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(MbedTLS REQUIRED)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
find_package(ZLIB REQUIRED)

if(NOT TARGET happy-eyeballs)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/happy-eyeballs" "${CMAKE_BINARY_DIR}/shared/happy-eyeballs")
endif()

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/opts-parser" "${CMAKE_BINARY_DIR}/shared/opts-parser")
endif()

if(NOT TARGET OBS::bpm)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/bpm" bpm)
endif()

add_library(obs-outputs MODULE)
add_library(OBS::outputs ALIAS obs-outputs)

target_sources(
  obs-outputs
  PRIVATE
    $<$<BOOL:${ENABLE_HEVC}>:rtmp-hevc.c>
    $<$<BOOL:${ENABLE_HEVC}>:rtmp-hevc.h>
    flv-mux.c
    flv-mux.h
    flv-output.c
    librtmp/amf.c
    librtmp/amf.h
    librtmp/bytes.h
    librtmp/cencode.c
    librtmp/cencode.h
    librtmp/handshake.h
    librtmp/hashswf.c
    librtmp/http.h
    librtmp/log.c
    librtmp/log.h
    librtmp/md5.c
    librtmp/md5.h
    librtmp/parseurl.c
    librtmp/rtmp.c
    librtmp/rtmp.h
    librtmp/rtmp_sys.h
    mp4-mux-internal.h
    mp4-mux.c
    mp4-mux.h
    mp4-output.c
    net-if.c
    net-if.h
    null-output.c
    obs-output-ver.h
    obs-outputs.c
    rtmp-av1.c
    rtmp-av1.h
    rtmp-helpers.h
    rtmp-stream.c
    rtmp-stream.h
    rtmp-windows.c
    utils.h
)

target_compile_definitions(obs-outputs PRIVATE USE_MBEDTLS CRYPTO)

target_compile_options(
  obs-outputs
  PRIVATE
    $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-comma>
    $<$<AND:$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>,$<COMPILE_LANG_AND_ID:C,Clang>>:-Wno-error=unreachable-code>
)

target_link_libraries(
  obs-outputs
  PRIVATE
    OBS::libobs
    OBS::happy-eyeballs
    OBS::opts-parser
    OBS::bpm
    MbedTLS::mbedtls
    ZLIB::ZLIB
    $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>
    $<$<PLATFORM_ID:Windows>:crypt32>
    $<$<PLATFORM_ID:Windows>:iphlpapi>
    $<$<PLATFORM_ID:Windows>:winmm>
    $<$<PLATFORM_ID:Windows>:ws2_32>
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Foundation.framework>>"
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Security.framework>>"
)

# Remove once jansson has been fixed on obs-deps
target_link_options(
  obs-outputs
  PRIVATE $<$<PLATFORM_ID:Windows>:/IGNORE:4098> $<$<AND:$<PLATFORM_ID:Windows>,$<CONFIG:DEBUG>>:/NODEFAULTLIB:MSVCRT>
)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in obs-outputs.rc)
  target_sources(obs-outputs PRIVATE obs-outputs.rc)
endif()

set_target_properties_obs(obs-outputs PROPERTIES FOLDER plugins/obs-outputs PREFIX "")
