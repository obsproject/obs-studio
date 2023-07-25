cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

find_package(MbedTLS REQUIRED)
find_package(ZLIB REQUIRED)

add_library(obs-outputs MODULE)
add_library(OBS::outputs ALIAS obs-outputs)

target_sources(
  obs-outputs
  PRIVATE obs-outputs.c
          obs-output-ver.h
          flv-mux.c
          flv-mux.h
          flv-output.c
          net-if.c
          net-if.h
          null-output.c
          rtmp-helpers.h
          rtmp-stream.c
          rtmp-stream.h
          rtmp-windows.c
          rtmp-av1.c
          rtmp-av1.h
          utils.h
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
          "$<$<BOOL:${ENABLE_HEVC}>:rtmp-hevc.c>"
          "$<$<BOOL:${ENABLE_HEVC}>:rtmp-hevc.h>")

if(NOT TARGET happy-eyeballs)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/happy-eyeballs" "${CMAKE_BINARY_DIR}/deps/happy-eyeballs")
endif()

target_link_libraries(obs-outputs PRIVATE OBS::libobs OBS::happy-eyeballs MbedTLS::MbedTLS ZLIB::ZLIB)

target_compile_definitions(obs-outputs PRIVATE USE_MBEDTLS CRYPTO)

if(OS_WINDOWS)
  target_link_libraries(obs-outputs PRIVATE OBS::w32-pthreads ws2_32 winmm Iphlpapi crypt32)
  target_link_options(obs-outputs PRIVATE /IGNORE:4098 /IGNORE:4099)

  configure_file(cmake/windows/obs-module.rc.in obs-outputs.rc)

  target_sources(obs-outputs PRIVATE obs-outputs.rc)
elseif(OS_MACOS)
  target_compile_options(obs-outputs PRIVATE -Wno-comma)
  # cmake-format: off
  target_link_libraries(
    obs-outputs
    PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Foundation.framework>"
            "$<LINK_LIBRARY:FRAMEWORK,Security.framework>")
  # cmake-format: on
endif()

include(cmake/ftl.cmake)

set_target_properties_obs(obs-outputs PROPERTIES FOLDER plugins PREFIX "")
