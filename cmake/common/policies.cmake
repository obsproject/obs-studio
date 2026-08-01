# OBS CMake policies module

include_guard(GLOBAL)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.2.0)
  cmake_policy(SET CMP0200 NEW)
endif()
