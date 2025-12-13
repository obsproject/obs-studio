# CMake common compiler options module

include_guard(GLOBAL)

# Set C and C++ language standards to C17 and C++17
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.21)
  set(CMAKE_C_STANDARD 17)
else()
  set(CMAKE_C_STANDARD 11)
endif()
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Set symbols to be hidden by default for C and C++
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

# clang options for C
set(_obs_clang_c_options
    # cmake-format: sortable
    -fno-strict-aliasing
    -Wbool-conversion
    -Wcomma
    -Wconstant-conversion
    -Wdeprecated-declarations
    -Wempty-body
    -Wenum-conversion
    -Werror=return-type
    -Wextra
    -Wformat
    -Wformat-security
    -Wfour-char-constants
    -Winfinite-recursion
    -Wint-conversion
    -Wnewline-eof
    -Wno-conversion
    -Wno-float-conversion
    -Wno-implicit-fallthrough
    -Wno-missing-braces
    -Wno-missing-field-initializers
    -Wno-missing-prototypes
    -Wno-semicolon-before-method-body
    -Wno-shadow
    -Wno-sign-conversion
    -Wno-strict-prototypes
    -Wno-trigraphs
    -Wno-unknown-pragmas
    -Wno-unused-function
    -Wno-unused-label
    -Wnon-literal-null-conversion
    -Wobjc-literal-conversion
    -Wparentheses
    -Wpointer-sign
    -Wquoted-include-in-framework-header
    -Wshadow
    -Wshorten-64-to-32
    -Wuninitialized
    -Wunreachable-code
    -Wunused-parameter
    -Wunused-value
    -Wunused-variable
    -Wvla)

# clang options for C++
set(_obs_clang_cxx_options
    # cmake-format: sortable
    ${_obs_clang_c_options}
    -Wconversion
    -Wdeprecated-implementations
    -Wduplicate-method-match
    -Wfloat-conversion
    -Wfour-char-constants
    -Wimplicit-retain-self
    -Winvalid-offsetof
    -Wmove
    -Wno-c++11-extensions
    -Wno-exit-time-destructors
    -Wno-implicit-atomic-properties
    -Wno-objc-interface-ivars
    -Wno-overloaded-virtual
    -Wrange-loop-analysis)

if(NOT DEFINED CMAKE_COMPILE_WARNING_AS_ERROR)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()
