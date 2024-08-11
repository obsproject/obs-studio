add_custom_command(
  TARGET inject-helper
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t inject-helper
  COMMENT "Build 32-bit inject-helper")
