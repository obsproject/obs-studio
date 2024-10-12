# OBS CMake Windows compiler configuration module

include_guard(GLOBAL)

include(ccache)
include(compiler_common)

if(ENABLE_CCACHE AND CCACHE_PROGRAM)
  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    file(COPY_FILE ${CCACHE_PROGRAM} "${CMAKE_CURRENT_BINARY_DIR}/cl.exe")
    set(
      CMAKE_VS_GLOBALS
      "CLToolExe=cl.exe"
      "CLToolPath=${CMAKE_BINARY_DIR}"
      "TrackFileAccess=false"
      "UseMultiToolTask=true"
    )
    # Ccache does not support debug information stored in program database (PDB) files
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    file(COPY_FILE ${CCACHE_PROGRAM} "${CMAKE_CURRENT_BINARY_DIR}/clang-cl.exe")
    set(
      CMAKE_VS_GLOBALS
      "CLToolExe=clang-cl.exe"
      "CLToolPath=${CMAKE_BINARY_DIR}"
      "TrackFileAccess=false"
      "UseMultiToolTask=true"
    )
  endif()
endif()

# CMake 3.24 introduces a bug mistakenly interpreting MSVC as supporting the '-pthread' compiler flag
if(CMAKE_VERSION VERSION_EQUAL 3.24.0)
  set(THREADS_HAVE_PTHREAD_ARG FALSE)
endif()

# CMake 3.25 changed the way symbol generation is handled on Windows
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25.0)
  if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
  else()
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
  endif()
endif()

message(DEBUG "Current Windows API version: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM)
  message(DEBUG "Maximum Windows API version: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM}")
endif()

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_LESS 10.0.20348)
  message(
    FATAL_ERROR
    "OBS requires Windows SDK version 10.0.20348.0 or more recent.\n"
    "Please download and install the most recent Windows SDK."
  )
endif()

set(_obs_msvc_c_options /MP /Zc:__cplusplus /Zc:preprocessor)
set(_obs_msvc_cpp_options /MP /Zc:__cplusplus /Zc:preprocessor)

if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
  list(APPEND _obs_msvc_cpp_options /Zc:char8_t-)
endif()

add_compile_options(
  /W3
  /utf-8
  /Brepro
  /permissive-
  "$<$<COMPILE_LANG_AND_ID:C,MSVC>:${_obs_msvc_c_options}>"
  "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:${_obs_msvc_cpp_options}>"
  "$<$<COMPILE_LANG_AND_ID:C,Clang>:${_obs_clang_c_options}>"
  "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${_obs_clang_cxx_options}>"
  $<$<NOT:$<CONFIG:Debug>>:/Gy>
  $<$<NOT:$<CONFIG:Debug>>:/GL>
  $<$<NOT:$<CONFIG:Debug>>:/Oi>
)

add_compile_definitions(
  NOMINMAX
  UNICODE
  _UNICODE
  _CRT_SECURE_NO_WARNINGS
  _CRT_NONSTDC_NO_WARNINGS
  $<$<CONFIG:DEBUG>:DEBUG>
  $<$<CONFIG:DEBUG>:_DEBUG>
)

add_link_options(
  $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
  $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>
  $<$<NOT:$<CONFIG:Debug>>:/LTCG>
  $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
  /DEBUG
  /Brepro
)

if(CMAKE_COMPILE_WARNING_AS_ERROR)
  add_link_options(/WX)
endif()
