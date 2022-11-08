project(linux-virtualcam)

option(ENABLE_VIRTUALCAM "Build OBS Virtualcam (Linux)" ON)

if(NOT ENABLE_VIRTUALCAM)
  obs_status(DISABLED "linux-virtualcam")
  return()
endif()

find_package(ALSA REQUIRED)
find_package(Libv4l2 REQUIRED)

add_library(linux-virtualcam MODULE)
add_library(OBS::virtualcam ALIAS linux-virtualcam)

target_sources(linux-virtualcam PRIVATE alsa-output.c v4l2-output.c
                                        linux-virtualcam.c)

target_link_libraries(linux-virtualcam PRIVATE OBS::libobs LIB4L2::LIB4L2
                                               ALSA::ALSA)

set_target_properties(linux-virtualcam PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-virtualcam)
