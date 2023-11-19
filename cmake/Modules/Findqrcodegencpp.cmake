#[=======================================================================[.rst
Findqrcodegencpp
----------------

FindModule for qrcodegencpp and associated libraries

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``qrcodegencpp::qrcodegencpp``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``qrcodegencpp_FOUND``
  True, if all required components and the core library were found.
``qrcodegencpp_VERSION``
  Detected version of found qrcodegencpp libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``qrcodegencpp_LIBRARY``
  Path to the library component of qrcodegencpp.
``qrcodegencpp_INCLUDE_DIR``
  Directory containing ``qrcodegen.hpp``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_qrcodegencpp QUIET qrcodegencpp)
endif()

# qrcodegencpp_set_soname: Set SONAME on imported library target
macro(qrcodegencpp_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${qrcodegencpp_LIBRARY}' | grep -v '${qrcodegencpp_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET qrcodegencpp::qrcodegencpp PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "${CMAKE_OBJDUMP} -p '${qrcodegencpp_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET qrcodegencpp::qrcodegencpp PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

# qrcodegencpp_find_dll: Find DLL for corresponding import library
macro(qrcodegencpp_find_dll)
  cmake_path(GET qrcodegencpp_IMPLIB PARENT_PATH _implib_path)
  cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")

  string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" _dll_version "${qrcodegencpp_VERSION}")

  find_program(
    qrcodegencpp_LIBRARY
    NAMES qrcodegencpp.dll
    HINTS ${_implib_path} ${_bin_path}
    DOC "qrcodegencpp DLL location")

  if(NOT qrcodegencpp_LIBRARY)
    set(qrcodegencpp_LIBRARY "${qrcodegencpp_IMPLIB}")
  endif()
  unset(_implib_path)
  unset(_bin_path)
  unset(_dll_version)
endmacro()

find_path(
  qrcodegencpp_INCLUDE_DIR
  NAMES qrcodegen.hpp
  HINTS ${PC_qrcodegencpp_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES qrcodegencpp qrcodegen
  DOC "qrcodegencpp include directory")

if(PC_qrcodegencpp_VERSION VERSION_GREATER 0)
  set(qrcodegencpp_VERSION ${PC_qrcodegencpp_VERSION})
else()
  if(NOT qrcodegencpp_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find qrcodegencpp version.")
  endif()
  set(qrcodegencpp_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  find_library(
    qrcodegencpp_IMPLIB
    NAMES qrcodegencpp qrcodegencpp
    DOC "qrcodegencpp import library location")

  qrcodegencpp_find_dll()
else()
  find_library(
    qrcodegencpp_LIBRARY
    NAMES qrcodegencpp qrcodegencpp
    HINTS ${PC_qrcodegencpp_LIBRARY_DIRS}
    PATHS /usr/lib /usr/local/lib
    DOC "qrcodegencpp location")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(qrcodegencpp_ERROR_REASON "Ensure that a qrcodegencpp distribution is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(qrcodegencpp_ERROR_REASON "Ensure that qrcodegencpp is installed on the system.")
endif()

find_package_handle_standard_args(
  qrcodegencpp
  REQUIRED_VARS qrcodegencpp_LIBRARY qrcodegencpp_INCLUDE_DIR
  VERSION_VAR qrcodegencpp_VERSION REASON_FAILURE_MESSAGE "${qrcodegencpp_ERROR_REASON}")
mark_as_advanced(qrcodegencpp_INCLUDE_DIR qrcodegencpp_LIBRARY qrcodegencpp_IMPLIB)
unset(qrcodegencpp_ERROR_REASON)

if(qrcodegencpp_FOUND)
  if(NOT TARGET qrcodegencpp::qrcodegencpp)
    if(IS_ABSOLUTE "${qrcodegencpp_LIBRARY}")
      if(DEFINED qrcodegencpp_IMPLIB)
        if(qrcodegencpp_IMPLIB STREQUAL qrcodegencpp_LIBRARY)
          add_library(qrcodegencpp::qrcodegencpp STATIC IMPORTED)
        else()
          add_library(qrcodegencpp::qrcodegencpp SHARED IMPORTED)
          set_property(TARGET qrcodegencpp::qrcodegencpp PROPERTY IMPORTED_IMPLIB "${qrcodegencpp_IMPLIB}")
        endif()
      else()
        add_library(qrcodegencpp::qrcodegencpp UNKNOWN IMPORTED)
      endif()
      set_property(TARGET qrcodegencpp::qrcodegencpp PROPERTY IMPORTED_LOCATION "${qrcodegencpp_LIBRARY}")
    else()
      add_library(qrcodegencpp::qrcodegencpp INTERFACE IMPORTED)
      set_property(TARGET qrcodegencpp::qrcodegencpp PROPERTY IMPORTED_LIBNAME "${qrcodegencpp_LIBRARY}")
    endif()

    qrcodegencpp_set_soname()
    set_target_properties(
      qrcodegencpp::qrcodegencpp
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_qrcodegencpp_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${qrcodegencpp_INCLUDE_DIR}"
                 VERSION ${qrcodegencpp_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  qrcodegencpp PROPERTIES
  URL "https://www.nayuki.io/page/qr-code-generator-library"
  DESCRIPTION "This project aims to be the best, clearest library for generating QR Codes in C++.")
