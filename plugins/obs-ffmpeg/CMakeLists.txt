project(obs-ffmpeg)

option(ENABLE_FFMPEG_LOGGING "Enables obs-ffmpeg logging" OFF)
option(ENABLE_NEW_MPEGTS_OUTPUT "Use native SRT/RIST mpegts output" ON)

find_package(
  FFmpeg REQUIRED
  COMPONENTS avcodec
             avfilter
             avdevice
             avutil
             swscale
             avformat
             swresample)

add_library(obs-ffmpeg MODULE)
add_library(OBS::ffmpeg ALIAS obs-ffmpeg)

add_subdirectory(ffmpeg-mux)
if(ENABLE_NEW_MPEGTS_OUTPUT)
  find_package(Librist QUIET)
  find_package(Libsrt QUIET)

  if(NOT TARGET Librist::Librist AND NOT TARGET Libsrt::Libsrt)
    obs_status(
      FATAL_ERROR
      "SRT and RIST libraries not found! Please install SRT and RIST libraries or set ENABLE_NEW_MPEGTS_OUTPUT=OFF."
    )
  elseif(NOT TARGET Libsrt::Libsrt)
    obs_status(
      FATAL_ERROR
      "SRT library not found! Please install SRT library or set ENABLE_NEW_MPEGTS_OUTPUT=OFF."
    )
  elseif(NOT TARGET Librist::Librist)
    obs_status(
      FATAL_ERROR
      "RIST library not found! Please install RIST library or set ENABLE_NEW_MPEGTS_OUTPUT=OFF."
    )
  endif()
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/obs-ffmpeg-config.h.in
               ${CMAKE_BINARY_DIR}/config/obs-ffmpeg-config.h)

target_sources(
  obs-ffmpeg
  PRIVATE obs-ffmpeg.c
          obs-ffmpeg-video-encoders.c
          obs-ffmpeg-audio-encoders.c
          obs-ffmpeg-av1.c
          obs-ffmpeg-nvenc.c
          obs-ffmpeg-output.c
          obs-ffmpeg-mux.c
          obs-ffmpeg-mux.h
          obs-ffmpeg-hls-mux.c
          obs-ffmpeg-source.c
          obs-ffmpeg-compat.h
          obs-ffmpeg-formats.h
          ${CMAKE_BINARY_DIR}/config/obs-ffmpeg-config.h)

target_include_directories(obs-ffmpeg PRIVATE ${CMAKE_BINARY_DIR}/config)

target_link_libraries(
  obs-ffmpeg
  PRIVATE OBS::libobs
          OBS::media-playback
          OBS::opts-parser
          FFmpeg::avcodec
          FFmpeg::avfilter
          FFmpeg::avformat
          FFmpeg::avdevice
          FFmpeg::avutil
          FFmpeg::swscale
          FFmpeg::swresample)

if(ENABLE_NEW_MPEGTS_OUTPUT)
  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg-mpegts.c obs-ffmpeg-srt.h
                                    obs-ffmpeg-rist.h obs-ffmpeg-url.h)

  target_link_libraries(obs-ffmpeg PRIVATE Librist::Librist Libsrt::Libsrt)
  if(OS_WINDOWS)
    target_link_libraries(obs-ffmpeg PRIVATE ws2_32.lib)
  endif()
  target_compile_definitions(obs-ffmpeg PRIVATE NEW_MPEGTS_OUTPUT)
endif()

if(ENABLE_FFMPEG_LOGGING)
  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg-logging.c)
endif()

set_target_properties(obs-ffmpeg PROPERTIES FOLDER "plugins/obs-ffmpeg" PREFIX
                                                                        "")
target_compile_options(
  obs-ffmpeg
  PRIVATE
    $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:-Wno-switch>
)

if(OS_WINDOWS)
  add_subdirectory(obs-amf-test)
  add_subdirectory(obs-nvenc-test)

  if(MSVC)
    target_link_libraries(obs-ffmpeg PRIVATE OBS::w32-pthreads)
  endif()

  set(MODULE_DESCRIPTION "OBS FFmpeg module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 obs-ffmpeg.rc)

  target_sources(
    obs-ffmpeg
    PRIVATE texture-amf.cpp
            texture-amf-opts.hpp
            jim-nvenc.c
            jim-nvenc.h
            jim-nvenc-helpers.c
            jim-nvenc-ver.h
            obs-ffmpeg.rc)

elseif(OS_POSIX AND NOT OS_MACOS)
  find_package(Libva REQUIRED)
  find_package(Libpci REQUIRED)
  target_sources(obs-ffmpeg PRIVATE obs-ffmpeg-vaapi.c vaapi-utils.c
                                    vaapi-utils.h)
  target_link_libraries(obs-ffmpeg PRIVATE Libva::va LIBPCI::LIBPCI)
endif()

setup_plugin_target(obs-ffmpeg)
