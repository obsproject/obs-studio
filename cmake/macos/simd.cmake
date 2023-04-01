# OBS CMake macOS SIMD module

# Enable openmp-simd when compiling for arm64
if(CMAKE_OSX_ARCHITECTURES MATCHES ".*[aA][rR][mM]64e?.*" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(ARCH_SIMD_FLAGS -fopenmp-simd)
  set(ARCH_SIMD_DEFINES SIMDE_ENABLE_OPENMP)
endif()
