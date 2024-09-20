add_custom_command(
  TARGET obs-virtualcam-module
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t obs-virtualcam-module
  COMMENT "Build 32-bit obs-virtualcam")
