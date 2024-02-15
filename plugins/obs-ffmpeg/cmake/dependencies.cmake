# cmake-format: off
if(OS_WINDOWS OR OS_MACOS)
  set(ffmpeg_version 6)
else()
  set(ffmpeg_version 4.4)
endif()

find_package(
  FFmpeg ${ffmpeg_version}
  REQUIRED avcodec
           avfilter
           avdevice
           avutil
           swscale
           avformat
           swresample)
# cmake-format: on

if(NOT TARGET OBS::media-playback)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/media-playback" "${CMAKE_BINARY_DIR}/deps/media-playback")
endif()

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/opts-parser" "${CMAKE_BINARY_DIR}/deps/opts-parser")
endif()

if(OS_WINDOWS)
  find_package(AMF 1.4.29 REQUIRED)
  find_package(FFnvcodec 12 REQUIRED)

  add_library(obs-nvenc-version INTERFACE)
  add_library(OBS::obs-nvenc-version ALIAS obs-nvenc-version)
  target_sources(obs-nvenc-version INTERFACE obs-nvenc-ver.h)
  target_include_directories(obs-nvenc-version INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

  add_subdirectory(obs-amf-test)
  add_subdirectory(obs-nvenc-test)
elseif(
  OS_LINUX
  OR OS_FREEBSD
  OR OS_OPENBSD)
  find_package(Libva REQUIRED)
  find_package(Libpci REQUIRED)
endif()

if(ENABLE_NEW_MPEGTS_OUTPUT)
  find_package(Librist QUIET)
  find_package(Libsrt QUIET)

  foreach(_output_lib IN ITEMS Librist Libsrt)
    if(NOT TARGET ${_output_lib}::${_output_lib})
      list(APPEND _error_messages "MPEGTS output library ${_output_lib} not found.")
    endif()
  endforeach()

  if(_error_messages)
    list(JOIN _error_messages "\n" _error_string)
    message(
      FATAL_ERROR
        "${_error_string}\n Disable this error by setting ENABLE_NEW_MPEGTS_OUTPUT to OFF or providing the build system with required SRT and Rist libraries."
    )
  endif()
endif()
