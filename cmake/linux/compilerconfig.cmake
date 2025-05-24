# OBS CMake Linux compiler configuration module

include_guard(GLOBAL)

include(ccache)
include(compiler_common)

option(ENABLE_COMPILER_TRACE "Enable Clang time-trace (requires Clang and Ninja)" OFF)
mark_as_advanced(ENABLE_COMPILER_TRACE)

# gcc options for C
set(
  _obs_gcc_c_options
  $<$<BOOL:${OBS_COMPILE_DEPRECATION_AS_WARNING}>:-Wno-error=deprecated-declarations>
  -fno-strict-aliasing
  -fopenmp-simd
  -Wdeprecated-declarations
  -Wempty-body
  -Wenum-conversion
  -Werror=return-type
  -Wextra
  -Wformat
  -Wformat-security
  -Wno-conversion
  -Wno-float-conversion
  -Wno-implicit-fallthrough
  -Wno-missing-braces
  -Wno-missing-field-initializers
  -Wno-shadow
  -Wno-sign-conversion
  -Wno-trigraphs
  -Wno-unknown-pragmas
  -Wno-unused-function
  -Wno-unused-label
  -Wparentheses
  -Wswitch
  -Wuninitialized
  -Wunreachable-code
  -Wunused-parameter
  -Wunused-value
  -Wunused-variable
  -Wvla
)

add_compile_options(
  -fopenmp-simd
  "$<$<COMPILE_LANG_AND_ID:C,GNU>:${_obs_gcc_c_options}>"
  "$<$<COMPILE_LANG_AND_ID:C,GNU>:-Wint-conversion;-Wno-missing-prototypes;-Wno-strict-prototypes;-Wpointer-sign>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${_obs_gcc_c_options}>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Winvalid-offsetof;-Wno-overloaded-virtual>"
  "$<$<COMPILE_LANG_AND_ID:C,Clang>:${_obs_clang_c_options}>"
  "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${_obs_clang_cxx_options}>"
)

# CMake switch for warnings as errors to CMake < 3.24
if(CMAKE_VERSION VERSION_LESS 3.24.0)
  if(CMAKE_COMPILE_WARNING_AS_ERROR)
    add_compile_options(-Werror)
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  # * Disable false-positive warning in GCC 12.1.0 and later
  # * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105562
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12.1.0)
    add_compile_options(-Wno-error=maybe-uninitialized)
  endif()

  # * Add warning for infinite recursion (added in GCC 12)
  # * Also disable warnings for stringop-overflow due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106297
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0.0)
    add_compile_options(-Winfinite-recursion -Wno-stringop-overflow)
  endif()

  if(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
    add_compile_options(-Wno-error=type-limits)
  endif()
endif()

# Enable compiler and build tracing (requires Ninja generator)
if(ENABLE_COMPILER_TRACE AND CMAKE_GENERATOR STREQUAL "Ninja")
  add_compile_options($<$<COMPILE_LANG_AND_ID:C,Clang>:-ftime-trace> $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-ftime-trace>)
else()
  set(ENABLE_COMPILER_TRACE OFF CACHE STRING "Enable Clang time-trace (required Clang and Ninja)" FORCE)
endif()

add_compile_definitions($<$<CONFIG:DEBUG>:DEBUG> $<$<CONFIG:DEBUG>:_DEBUG> SIMDE_ENABLE_OPENMP)
