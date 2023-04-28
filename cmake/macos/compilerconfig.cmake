# OBS CMake macOS compiler configuration module

include_guard(GLOBAL)

include(ccache)
include(compiler_common)
include(simd)

# Set C17 / C++17 standards as required and disable extensions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set symbols to be hidden by default for C and C++
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

# Add default C and C++ compiler options if Xcode generator is not used
if(NOT XCODE)
  set(_obs_common_options
      -fno-strict-aliasing
      -Werror
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
      -Wno-error=shorten-64-to-32)

  set(_obs_c_options ${_obs_common_options} -Wno-shadow -Wno-float-conversion)

  set(_obs_cxx_options
      ${_obs_common_options}
      -Wno-non-virtual-dtor
      -Wno-overloaded-virtual
      -Wno-exit-time-destructors
      -Wno-shadow
      -Winvalid-offsetof
      -Wmove
      -Werror=block-capture-autoreleasing
      -Wrange-loop-analysis)

  set(_obs_objc_options
      ${_obs_common_options}
      -Wno-implicit-atomic-properties
      -Wno-objc-interface-ivars
      -Warc-repeated-use-of-weak
      -Wno-arc-maybe-repeated-use-of-weak
      -Wimplicit-retain-self
      -Wduplicate-method-match
      -Wshadow
      -Wfloat-conversion
      -Wobjc-literal-conversion
      -Wno-selector
      -Wno-strict-selector-match
      -Wundeclared-selector
      -Wdeprecated-implementations
      -Wprotocol
      -Werror=block-capture-autoreleasing
      -Wrange-loop-analysis)

  set(_obs_objcxx_options ${_obs_objc_options} -Wno-non-virtual-dtor)

  # Enable stripping of dead symbols when not building for Debug configuration
  set(_release_configs RelWithDebInfo Release MinSizeRel)
  if(CMAKE_BUILD_TYPE IN_LIST _release_configs)
    add_link_options(LINKER:-dead_strip)
  endif()

  add_compile_options(
    "$<$<COMPILE_LANGUAGE:C>:${_obs_c_options}>" "$<$<COMPILE_LANGUAGE:CXX>:${_obs_cxx_options}>"
    "$<$<COMPILE_LANGUAGE:OBJC>:${_obs_objc_options}>" "$<$<COMPILE_LANGUAGE:OBJCXX>:${_obs_objcxx_options}>")

  option(ENABLE_COMPILER_TRACE "Enable clang time-trace (requires Ninja)" OFF)
  mark_as_advanced(ENABLE_COMPILER_TRACE)

  # Add time trace option to compiler, if enabled.
  if(ENABLE_COMPILER_TRACE AND CMAKE_GENERATOR STREQUAL "Ninja")
    add_compile_options($<$<NOT:$<COMPILE_LANGUAGE:Swift>>:-ftime-trace>)
  else()
    set(ENABLE_COMPILER_TRACE
        OFF
        CACHE BOOL "Enable clang time-trace (requires Ninja)" FORCE)
  endif()

  # Enable color diagnostics for AppleClang
  set(CMAKE_COLOR_DIAGNOSTICS ON)
endif()

add_compile_definitions("$<$<AND:$<COMPILE_LANGUAGE:C,CXX,OBJC,OBJCXX>,$<CONFIG:DEBUG>>:DEBUG;_DEBUG>")
