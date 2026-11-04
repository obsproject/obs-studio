if(BUILD_TESTS OR ENABLE_UNIT_TESTS)
  foreach(test_name IN ITEMS test_preview_frame_selection test_preview_stats)
    add_executable(${test_name} ${test_name}.c)
    target_include_directories(${test_name} PRIVATE "${CMAKE_SOURCE_DIR}/libobs")
    add_test(NAME ${test_name} COMMAND ${test_name})
  endforeach()
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
