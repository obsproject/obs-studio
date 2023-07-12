add_custom_command(
  TARGET graphics-hook
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t graphics-hook
  COMMENT "Build 32-bit graphics-hook")
