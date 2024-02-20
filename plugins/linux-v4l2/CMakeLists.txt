project(linux-v4l2)

option(ENABLE_V4L2 "Build OBS with v4l2 support" ON)
if(NOT ENABLE_V4L2)
  obs_status(DISABLED "linux-v4l2")
  return()
endif()

option(ENABLE_UDEV "Build linux-v4l2 with UDEV support" ON)

find_package(Libv4l2 REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat)

add_library(linux-v4l2 MODULE)
add_library(OBS::v4l2 ALIAS linux-v4l2)

target_sources(linux-v4l2 PRIVATE linux-v4l2.c v4l2-controls.c v4l2-input.c v4l2-helpers.c v4l2-output.c v4l2-decoder.c)

target_link_libraries(linux-v4l2 PRIVATE OBS::libobs LIB4L2::LIB4L2 FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil)

set_target_properties(linux-v4l2 PROPERTIES FOLDER "plugins")

if(ENABLE_UDEV)
  find_package(Udev REQUIRED)
  target_sources(linux-v4l2 PRIVATE v4l2-udev.c)

  target_link_libraries(linux-v4l2 PRIVATE Udev::Udev)
  target_compile_definitions(linux-v4l2 PRIVATE HAVE_UDEV)
endif()

setup_plugin_target(linux-v4l2)
