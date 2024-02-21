# cmake-format: off
if(OS_WINDOWS OR OS_MACOS)
  set(ffmpeg_version 6)
else()
  set(ffmpeg_version 4.4)
endif()

find_package(
  FFmpeg ${ffmpeg_version}
  REQUIRED avcodec
           avutil
           avformat)
# cmake-format: on

find_qt(COMPONENTS Core Widgets Charts)
find_package(Librist QUIET)
find_package(Libsrt QUIET)

foreach(_output_lib IN ITEMS Librist Libsrt)
  if(NOT TARGET ${_output_lib}::${_output_lib})
    list(APPEND _error_messages "MPEGTS output library ${_output_lib} not found.")
  endif()
endforeach()

if(_error_messages)
  list(JOIN "\n" _error_string _error_string)
  message(
    FATAL_ERROR
      "${_error_string}\n Disable this error by setting ENABLE_NEW_MPEGTS_OUTPUT to OFF or providing the build system with required SRT and Rist libraries."
  )
endif()
