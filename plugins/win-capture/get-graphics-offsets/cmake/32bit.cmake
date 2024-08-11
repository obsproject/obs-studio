add_custom_command(
  TARGET get-graphics-offsets
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_SOURCE_DIR}/build_x86 --config $<CONFIG> -t get-graphics-offsets
  COMMENT "Build 32-bit get-graphics-offsets")
