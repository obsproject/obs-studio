include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compile options for MSVC
if(OS_WINDOWS AND MSVC)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user")
    file(
      GENERATE
      OUTPUT "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user"
      INPUT "${CMAKE_SOURCE_DIR}/cmake/bundle/windows/ALL_BUILD.vcxproj.user.in"
    )
  endif()

  # CMake 3.24 introduces a bug mistakenly interpreting MSVC as supporting
  # `-pthread`
  if(${CMAKE_VERSION} VERSION_EQUAL "3.24.0")
    set(THREADS_HAVE_PTHREAD_ARG OFF)
  endif()

  # Check for Win SDK version 10.0.20348 or above
  obs_status(
    STATUS "Windows API version is ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
  string(REPLACE "." ";" WINAPI_VER
                 "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

  list(GET WINAPI_VER 0 WINAPI_VER_MAJOR)
  list(GET WINAPI_VER 1 WINAPI_VER_MINOR)
  list(GET WINAPI_VER 2 WINAPI_VER_BUILD)

  set(WINAPI_COMPATIBLE FALSE)
  if(WINAPI_VER_MAJOR EQUAL 10)
    if(WINAPI_VER_MINOR EQUAL 0)
      if(WINAPI_VER_BUILD GREATER_EQUAL 20348)
        set(WINAPI_COMPATIBLE TRUE)
      endif()
    else()
      set(WINAPI_COMPATIBLE TRUE)
    endif()
  elseif(WINAPI_VER_MAJOR GREATER 10)
    set(WINAPI_COMPATIBLE TRUE)
  endif()

  if(NOT WINAPI_COMPATIBLE)
    obs_status(
      FATAL_ERROR
      "OBS requires Windows 10 SDK version 10.0.20348.0 and above to compile.\n"
      "Please download the most recent Windows 10 SDK in order to compile.")
  endif()

  add_compile_options(
    /MP
    /W3
    /WX
    /wd4127
    /wd4201
    /wd4456
    /wd4457
    /wd4458
    /wd4459
    /wd4595
    $<$<CONFIG:RELWITHDEBINFO>:/Ob2>)

  add_compile_definitions(UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS
                          _CRT_NONSTDC_NO_WARNINGS)

  add_link_options(
    LINKER:/OPT:REF
    LINKER:/WX
    "$<$<NOT:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>:LINKER\:/SAFESEH\:NO>"
    "$<$<CONFIG:DEBUG>:LINKER\:/INCREMENTAL\:NO>"
    "$<$<CONFIG:RELWITHDEBINFO>:LINKER\:/INCREMENTAL\:NO;/OPT\:ICF>")
else()
  if(NOT DEFINED CCACHE_SUPPORT)
    find_program(CCACHE_PROGRAM "ccache")
    set(CCACHE_SUPPORT
        ON
        CACHE BOOL "Enable ccache support")
    mark_as_advanced(CCACHE_PROGRAM)
  endif()
  if(CCACHE_PROGRAM AND CCACHE_SUPPORT)
    set(CMAKE_CXX_COMPILER_LAUNCHER
        ${CCACHE_PROGRAM}
        CACHE INTERNAL "")
    set(CMAKE_C_COMPILER_LAUNCHER
        ${CCACHE_PROGRAM}
        CACHE INTERNAL "")
    set(CMAKE_OBJC_COMPILER_LAUNCHER
        ${CCACHE_PROGRAM}
        CACHE INTERNAL "")
    set(CMAKE_OBJCXX_COMPILER_LAUNCHER
        ${CCACHE_PROGRAM}
        CACHE INTERNAL "")
    set(CMAKE_CUDA_COMPILER_LAUNCHER
        ${CCACHE_PROGRAM}
        CACHE INTERNAL "") # CMake 3.9+
  else()
    unset(CMAKE_CXX_COMPILER_LAUNCHER CACHE)
    unset(CMAKE_C_COMPILER_LAUNCHER CACHE)
    unset(CMAKE_OBJC_COMPILER_LAUNCHER CACHE)
    unset(CMAKE_OBJCXX_COMPILER_LAUNCHER CACHE)
    unset(CMAKE_CUDA_COMPILER_LAUNCHER CACHE)
  endif()

  add_compile_options(
    -Wextra
    -Wvla
    -Wno-unused-function
    -Wno-missing-field-initializers
    -fno-strict-aliasing
    "$<$<COMPILE_LANGUAGE:C>:-Werror-implicit-function-declaration;-Wno-missing-braces>"
  )

  if(CMAKE_VERSION GREATER_EQUAL 3.24.0)
    set(CMAKE_COLOR_DIAGNOSTICS ON)
  else()
    add_compile_options(
      $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-fcolor-diagnostics>
      $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-fcolor-diagnostics>)
  endif()

  if(OS_MACOS AND OBS_CODESIGN_LINKER)
    add_link_options(LINKER:-adhoc_codesign)
  endif()

  if(MINGW)
    set(CMAKE_WIDL
        "widl"
        CACHE INTERNAL "wine IDL header file generation program")
    add_compile_definitions("_WIN32_WINNT=0x0600;WINVER=0x0600")
  endif()
endif()

add_compile_definitions("$<$<CONFIG:DEBUG>:DEBUG=1>"
                        "$<$<CONFIG:DEBUG>:_DEBUG=1>")

if(MSVC_CXX_ARCHITECTURE_ID)
  string(TOLOWER ${MSVC_CXX_ARCHITECTURE_ID} _HOST_ARCH)
else()
  string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _HOST_ARCH)
endif()

if(OS_MACOS)
  list(FIND CMAKE_OSX_ARCHITECTURES arm64 _HAS_APPLE_TARGET)
  if(_HAS_APPLE_TARGET GREATER_EQUAL 0 OR _HOST_ARCH STREQUAL arm64)
    set(ARCH_SIMD_FLAGS -fopenmp-simd)
    set(ARCH_SIMD_DEFINES SIMDE_ENABLE_OPENMP)
  endif()
  unset(_HAS_APPLE_TARGET)
elseif(_HOST_ARCH MATCHES "(i[3-6]86|x86|x64|x86_64|amd64|e2k)")
  if(NOT MSVC)
    set(ARCH_SIMD_FLAGS -mmmx -msse -msse2)
  endif()
elseif(_HOST_ARCH MATCHES "^(powerpc|ppc)64(le)?")
  set(ARCH_SIMD_DEFINES NO_WARN_X86_INTRINSICS)
  set(ARCH_SIMD_FLAGS -mvsx)
else()
  set(ARCH_SIMD_DEFINES SIMDE_ENABLE_OPENMP)
  check_c_compiler_flag("-fopenmp-simd" C_COMPILER_SUPPORTS_OPENMP_SIMD)
  check_cxx_compiler_flag("-fopenmp-simd" CXX_COMPILER_SUPPORTS_OPENMP_SIMD)

  foreach(_LANG C CXX)
    if(CMAKE_${_LANG}_COMPILER_ID MATCHES "^(Apple)?Clang|GNU"
       AND ${_LANG}_COMPILER_SUPPORTS_OPENMP_SIMD)
      list(APPEND ARCH_SIMD_FLAGS $<$<COMPILE_LANGUAGE:${_LANG}>:-fopenmp-simd>)
    endif()
  endforeach()
endif()

if(_HOST_ARCH MATCHES "e2k")
  foreach(
    TEST_C_FLAG
    "-Wno-unused-parameter"
    "-Wno-ignored-qualifiers"
    "-Wno-pointer-sign"
    "-Wno-unused-variable"
    "-Wno-sign-compare"
    "-Wno-bad-return-value-type"
    "-Wno-maybe-uninitialized")
    check_c_compiler_flag(${TEST_C_FLAG}
                          C_COMPILER_SUPPORTS_FLAG_${TEST_C_FLAG})
    if(C_COMPILER_SUPPORTS_FLAG_${TEST_C_FLAG})
      set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} ${TEST_C_FLAG})
    endif()
  endforeach()
  foreach(TEST_CXX_FLAG "-Wno-invalid-offsetof" "-Wno-maybe-uninitialized")
    check_cxx_compiler_flag(${TEST_CXX_FLAG}
                            CXX_COMPILER_SUPPORTS_FLAG_${TEST_CXX_FLAG})
    if(CXX_COMPILER_SUPPORTS_FLAG_${TEST_CXX_FLAG})
      set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${TEST_CXX_FLAG})
    endif()
  endforeach()
endif()
