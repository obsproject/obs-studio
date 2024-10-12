# OBS CMake common compiler options module

include_guard(GLOBAL)

option(OBS_COMPILE_DEPRECATION_AS_WARNING "Downgrade deprecation warnings to actual warnings" FALSE)
mark_as_advanced(OBS_COMPILE_DEPRECATION_AS_WARNING)

# Set C and C++ language standards to C17 and C++17
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Set symbols to be hidden by default for C and C++
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

# clang options for C, C++, ObjC, and ObjC++
set(
  _obs_clang_common_options
  -fno-strict-aliasing
  -Wno-trigraphs
  -Wno-missing-field-initializers
  -Wno-missing-prototypes
  -Werror=return-type
  -Wunreachable-code
  -Wquoted-include-in-framework-header
  -Wno-missing-braces
  -Wparentheses
  -Wswitch
  -Wno-unused-function
  -Wno-unused-label
  -Wunused-parameter
  -Wunused-variable
  -Wunused-value
  -Wempty-body
  -Wuninitialized
  -Wno-unknown-pragmas
  -Wfour-char-constants
  -Wconstant-conversion
  -Wno-conversion
  -Wint-conversion
  -Wbool-conversion
  -Wenum-conversion
  -Wnon-literal-null-conversion
  -Wsign-compare
  -Wshorten-64-to-32
  -Wpointer-sign
  -Wnewline-eof
  -Wno-implicit-fallthrough
  -Wdeprecated-declarations
  -Wno-sign-conversion
  -Winfinite-recursion
  -Wcomma
  -Wno-strict-prototypes
  -Wno-semicolon-before-method-body
  -Wformat-security
  -Wvla
  -Wno-error=shorten-64-to-32
  $<$<BOOL:${OBS_COMPILE_DEPRECATION_AS_WARNING}>:-Wno-error=deprecated-declarations>
)

# clang options for C
set(_obs_clang_c_options ${_obs_clang_common_options} -Wno-shadow -Wno-float-conversion)

# clang options for C++
set(
  _obs_clang_cxx_options
  ${_obs_clang_common_options}
  -Wno-non-virtual-dtor
  -Wno-overloaded-virtual
  -Wno-exit-time-destructors
  -Wno-shadow
  -Winvalid-offsetof
  -Wmove
  -Werror=block-capture-autoreleasing
  -Wrange-loop-analysis
)

if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
  list(APPEND _obs_clang_cxx_options -fno-char8_t)
endif()

if(NOT DEFINED CMAKE_COMPILE_WARNING_AS_ERROR)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

# Enable interprocedural optimization
message(STATUS "Checking for interprocedural optimization support")
if(NOT DEFINED HAS_INTERPROCEDURAL_OPTIMIZATION)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT _ipo_result OUTPUT _ipo_output)
  set(
    HAS_INTERPROCEDURAL_OPTIMIZATION
    ${_ipo_result}
    CACHE BOOL
    "Result of compiler check for interprocedural optimization"
    FORCE
  )

  if(HAS_INTERPROCEDURAL_OPTIMIZATION)
    message(STATUS "Checking for interprocedural optimization support - available")
  else()
    message(STATUS "Checking for interprocedural optimization support - unavailable")
  endif()

  mark_as_advanced(HAS_INTERPROCEDURAL_OPTIMIZATION)
  unset(_ipo_result)
  unset(_ipo_output)
endif()

if(HAS_INTERPROCEDURAL_OPTIMIZATION)
  message(STATUS "Checking for interprocedural optimization support - enabled [Release, MinSizeRel]")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
else()
  message(STATUS "Checking for interprocedural optimization support - disabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL OFF)
endif()
