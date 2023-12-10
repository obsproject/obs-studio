cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

if(POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if(POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

if(POLICY CMP0094)
  cmake_policy(SET CMP0094 NEW)
endif()

find_package(Python 3.8...<3.12 REQUIRED Interpreter Development)
find_package(SWIG 4 REQUIRED)

include(UseSWIG)

set_source_files_properties(obspython.i PROPERTIES USE_TARGET_INCLUDE_DIRECTORIES TRUE
                                                   SWIG_FLAGS "$<IF:$<BOOL:${OS_LINUX}>,-py3,-py3-stable-abi>")

swig_add_library(
  obspython
  LANGUAGE python
  TYPE MODULE
  SOURCES obspython.i)

add_library(OBS::python ALIAS obspython)

file(
  GENERATE
  OUTPUT $<$<PLATFORM_ID:Windows>:$<CONFIG>/>obspython.h
  CONTENT "#pragma once\n\n#define PYTHON_LIB \"$<TARGET_LINKER_FILE_NAME:Python::Python>\"\n")

target_include_directories(obspython PRIVATE "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>>")

list(APPEND _SWIG_DEFINITIONS "SWIG_TYPE_TABLE=obspython" "Py_ENABLE_SHARED=1" "SWIG_PYTHON_INTERPRETER_NO_DEBUG")

target_compile_definitions(obspython PRIVATE SWIG_TYPE_TABLE=obspython Py_ENABLE_SHARED=1
                                             SWIG_PYTHON_INTERPRETER_NO_DEBUG $<$<BOOL:${ENABLE_UI}>:ENABLE_UI>)

target_link_libraries(obspython PRIVATE OBS::scripting OBS::cstrcache OBS::libobs
                                        $<$<BOOL:${ENABLE_UI}>:OBS::frontend-api>)

if(OS_WINDOWS)
  target_link_libraries(obspython PRIVATE Python::Python)
  target_compile_options(obspython PRIVATE /wd4100 /wd4197)
elseif(OS_MACOS)
  get_target_property(_python_include_directory Python::Python INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(obspython PRIVATE ${_python_include_directory})
  target_compile_options(obspython PRIVATE -Wno-unused-parameter -Wno-macro-redefined -Wno-unreachable-code)
  target_link_options(obspython PUBLIC LINKER:-undefined,dynamic_lookup)

  set_property(TARGET obspython PROPERTY XCODE_ATTRIBUTE_STRIP_STYLE non-global)
elseif(OS_LINUX OR OS_FREEBSD)
  target_link_libraries(obspython PRIVATE Python::Python)
endif()

if(MSVC OR XCODE)
  add_custom_command(
    TARGET obspython
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/obspython.py"
            "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/obspython.py"
    VERBATIM)
endif()

set_property(
  TARGET obspython
  APPEND
  PROPERTY SWIG_COMPILE_DEFINITIONS "SWIG_TYPE_TABLE=obspython" "Py_ENABLE_SHARED=1" "SWIG_PYTHON_INTERPRETER_NO_DEBUG"
           "$<$<BOOL:${ENABLE_UI}>:ENABLE_UI>" "$<$<PLATFORM_ID:Windows>:MS_NO_COREDLL>")

set_target_properties_obs(obspython PROPERTIES FOLDER scripting PREFIX "_")
