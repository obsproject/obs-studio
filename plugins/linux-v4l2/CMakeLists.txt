cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

option(ENABLE_V4L2 "Build OBS with v4l2 support" ON)
option(ENABLE_UDEV "Build linux-v4l2 with UDEV support" ON)

if(NOT ENABLE_V4L2)
  target_disable(linux-v4l2)
  return()
endif()

find_package(Libv4l2 REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat)

if(OS_FREEBSD OR OS_OPENBSD)
  set(CMAKE_REQUIRED_INCLUDES "/usr/local/include")
endif()
include(CheckCSourceCompiles)
set(V4L_TEST_SOURCE "#include<linux/videodev2.h>\nint main(){return 0;}")
check_c_source_compiles("${V4L_TEST_SOURCE}" HAVE_VIDEODEV2_HEADER)

if(NOT HAVE_VIDEODEV2_HEADER)
  message(FATAL_ERROR "Required system header <linux/videodev2.h> not found.")
endif()

add_library(linux-v4l2 MODULE)
add_library(OBS::v4l2 ALIAS linux-v4l2)

target_sources(
  linux-v4l2
  PRIVATE linux-v4l2.c v4l2-controls.c v4l2-decoder.c v4l2-helpers.c v4l2-input.c v4l2-output.c
)

target_link_libraries(
  linux-v4l2
  PRIVATE OBS::libobs Libv4l2::Libv4l2 FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil
)

if(ENABLE_UDEV)
  find_package(Libudev REQUIRED)
  target_sources(linux-v4l2 PRIVATE v4l2-udev.c)

  target_link_libraries(linux-v4l2 PRIVATE Libudev::Libudev)
  target_compile_definitions(linux-v4l2 PRIVATE HAVE_UDEV)
endif()

set_target_properties_obs(linux-v4l2 PROPERTIES FOLDER plugins PREFIX "")
