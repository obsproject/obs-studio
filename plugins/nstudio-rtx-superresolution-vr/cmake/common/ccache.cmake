# CMake ccache module

include_guard(GLOBAL)

if(NOT DEFINED CCACHE_PROGRAM)
  message(DEBUG "Trying to find ccache on build host...")
  find_program(CCACHE_PROGRAM "ccache")
  mark_as_advanced(CCACHE_PROGRAM)
endif()

if(CCACHE_PROGRAM)
  message(DEBUG "Ccache found as ${CCACHE_PROGRAM}...")
  option(ENABLE_CCACHE "Enable compiler acceleration with ccache" ON)

  if(ENABLE_CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()
