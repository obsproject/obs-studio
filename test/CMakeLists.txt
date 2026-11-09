if(BUILD_TESTS OR ENABLE_UNIT_TESTS)
  add_executable(test_async_frame_buffer test_async_frame_buffer.c)
  target_include_directories(test_async_frame_buffer PRIVATE "${CMAKE_SOURCE_DIR}/libobs")
  add_test(NAME test_async_frame_buffer COMMAND test_async_frame_buffer)
endif()

if(BUILD_TESTS)
  add_subdirectory(test-input)

  if(OS_WINDOWS)
    add_subdirectory(win)
  endif()

  if(OS_MACOS)
    add_subdirectory(osx)
  endif()
endif()

if(ENABLE_UNIT_TESTS)
  add_subdirectory(cmocka)
endif()
