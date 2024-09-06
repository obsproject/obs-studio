cmake_minimum_required(VERSION 3.22...3.25)

if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

if(POLICY CMP0094)
  cmake_policy(SET CMP0094 NEW)
endif()

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(Python 3.8 REQUIRED Interpreter Development)
else()
  find_package(Python 3.8...<3.12 REQUIRED Interpreter Development)
endif()
find_package(SWIG 4 REQUIRED)

include(UseSWIG)

set_source_files_properties(
  obspython.i
  PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES TRUE SWIG_FLAGS $<$<PLATFORM_ID:Windows,Darwin>:-py3-stable-abi>
)

swig_add_library(obspython LANGUAGE python TYPE MODULE SOURCES obspython.i)

add_library(OBS::python ALIAS obspython)

file(
  GENERATE OUTPUT
  $<$<PLATFORM_ID:Windows>:$<CONFIG>/>obspython.h
  CONTENT "#pragma once\n\n#define PYTHON_LIB \"$<TARGET_LINKER_FILE_NAME:Python::Python>\"\n"
)

target_include_directories(
  obspython
  PRIVATE
    "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>>"
    "$<$<PLATFORM_ID:Darwin>:$<TARGET_PROPERTY:Python::Python,INTERFACE_INCLUDE_DIRECTORIES>>"
)

target_compile_options(
  obspython
  PRIVATE
    $<$<PLATFORM_ID:Windows>:/wd4100>
    $<$<PLATFORM_ID:Windows>:/wd4197>
    $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:-Wno-unused-parameter>
    $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-macro-redefined>
    $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-unreachable-code>
)

target_compile_definitions(
  obspython
  PRIVATE
    SWIG_TYPE_TABLE=obspython
    Py_ENABLE_SHARED=1
    SWIG_PYTHON_INTERPRETER_NO_DEBUG
    $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>
)

target_link_libraries(
  obspython
  PRIVATE
    OBS::cstrcache
    OBS::libobs
    OBS::scripting
    $<$<BOOL:${ENABLE_UI}>:OBS::frontend-api>
    $<$<NOT:$<PLATFORM_ID:Darwin>>:Python::Python>
)

target_link_options(obspython PRIVATE $<$<PLATFORM_ID:Darwin>:LINKER:-undefined,dynamic_lookup>)

if(MSVC OR XCODE)
  add_custom_command(
    TARGET obspython
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/obspython.py"
      "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/obspython.py"
    VERBATIM
  )
endif()

set_property(
  TARGET obspython
  APPEND
  PROPERTY
    SWIG_COMPILE_DEFINITIONS
      "SWIG_TYPE_TABLE=obspython"
      "Py_ENABLE_SHARED=1"
      "SWIG_PYTHON_INTERPRETER_NO_DEBUG"
      "$<$<BOOL:${ENABLE_UI}>:ENABLE_UI>"
      "$<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>"
)

set_target_properties_obs(obspython PROPERTIES FOLDER scripting PREFIX "_" XCODE_ATTRIBUTE_STRIP_STYLE non-global)
