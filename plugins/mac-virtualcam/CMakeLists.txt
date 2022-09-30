option(ENABLE_VIRTUALCAM "Build OBS Virtualcam" ON)

if(NOT ENABLE_VIRTUALCAM)
  obs_status(DISABLED "mac-virtualcam")
  return()
endif()

add_subdirectory(src/obs-plugin)
add_subdirectory(src/dal-plugin)
