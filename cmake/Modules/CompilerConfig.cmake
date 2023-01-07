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
    "$<$<CONFIG:DEBUG>:/DDEBUG=1;/D_DEBUG=1>"
    "$<$<CONFIG:RELWITHDEBINFO>:/Ob2>"
    /DUNICODE
    /D_UNICODE
    /D_CRT_SECURE_NO_WARNINGS
    /D_CRT_NONSTDC_NO_WARNINGS
    /utf-8)

  add_link_options(
    "LINKER:/OPT:REF"
    "LINKER:/WX"
    "$<$<NOT:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>>:LINKER\:/SAFESEH\:NO>"
    "$<$<CONFIG:DEBUG>:LINKER\:/INCREMENTAL\:NO>"
    "$<$<CONFIG:RELWITHDEBINFO>:LINKER\:/INCREMENTAL\:NO;/OPT:ICF>")
else()
  find_program(CCACHE_PROGRAM "ccache")
  set(CCACHE_SUPPORT
      ON
      CACHE BOOL "Enable ccache support")
  mark_as_advanced(CCACHE_PROGRAM)
  if(CCACHE_PROGRAM AND CCACHE_SUPPORT)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}") # CMake 3.9+
  endif()

  add_compile_options(
    -Werror
    -Wextra
    -Wvla
    -Wswitch
    -Wno-error=switch
    -Wformat
    -Wformat-security
    -Wunused-parameter
    -Wno-unused-function
    -Wno-missing-field-initializers
    -fno-strict-aliasing
    "$<$<COMPILE_LANGUAGE:C>:-Werror-implicit-function-declaration;-Wno-missing-braces>"
    "$<$<BOOL:${USE_LIBCXX}>:-stdlib=libc++>"
    "$<$<CONFIG:DEBUG>:-DDEBUG=1;-D_DEBUG=1>"
    "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wnull-conversion;-fcolor-diagnostics;-Wno-error=shorten-64-to-32>"
    "$<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wnull-conversion;-fcolor-diagnostics;-Wno-error=shorten-64-to-32>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wconversion-null>")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC on aarch64 emits type-limits warnings that do not appear on x86_64
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      add_compile_options(-Wno-error=type-limits)
    endif()

    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105562
    if(CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "12.1.0")
      add_compile_options(-Wno-error=maybe-uninitialized)
    endif()
  endif()

  if(OBS_CODESIGN_LINKER)
    add_link_options("LINKER:$<$<PLATFORM_ID:Darwin>:-adhoc_codesign>")
  endif()

  if(MINGW)
    set(CMAKE_WIDL
        "widl"
        CACHE INTERNAL "wine IDL header file generation program")
    add_compile_definitions("_WIN32_WINNT=0x0600;WINVER=0x0600")
  endif()
endif()

if(MSVC_CXX_ARCHITECTURE_ID)
  string(TOLOWER ${MSVC_CXX_ARCHITECTURE_ID} LOWERCASE_CMAKE_SYSTEM_PROCESSOR)
else()
  string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} LOWERCASE_CMAKE_SYSTEM_PROCESSOR)
endif()

if(LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES
   "(i[3-6]86|x86|x64|x86_64|amd64|e2k)")
  if(NOT MSVC AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(ARCH_SIMD_FLAGS -mmmx -msse -msse2)
  endif()
elseif(LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64(le)?")
  set(ARCH_SIMD_DEFINES -DNO_WARN_X86_INTRINSICS)
  set(ARCH_SIMD_FLAGS -mvsx)
else()
  if(CMAKE_C_COMPILER_ID MATCHES "^(Apple)?Clang|GNU"
     OR CMAKE_CXX_COMPILER_ID MATCHES "^(Apple)?Clang|GNU")
    check_c_compiler_flag("-fopenmp-simd" C_COMPILER_SUPPORTS_OPENMP_SIMD)
    check_cxx_compiler_flag("-fopenmp-simd" CXX_COMPILER_SUPPORTS_OPENMP_SIMD)
    set(ARCH_SIMD_FLAGS
        -DSIMDE_ENABLE_OPENMP
        "$<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:C_COMPILER_SUPPORTS_OPENMP_SIMD>>:-fopenmp-simd>"
        "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<BOOL:CXX_COMPILER_SUPPORTS_OPENMP_SIMD>>:-fopenmp-simd>"
    )
  endif()
endif()

if(LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "e2k")
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
