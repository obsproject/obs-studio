find_package(
  FFmpeg
  6.1
  REQUIRED avcodec avfilter avdevice avutil swscale avformat swresample
)

if(NOT TARGET OBS::media-playback)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/media-playback" "${CMAKE_BINARY_DIR}/shared/media-playback")
endif()

if(NOT TARGET OBS::opts-parser)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/opts-parser" "${CMAKE_BINARY_DIR}/shared/opts-parser")
endif()

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(Libva REQUIRED)
  find_package(Libpci REQUIRED)
  find_package(Libdrm REQUIRED)
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
