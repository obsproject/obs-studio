#[=======================================================================[.rst
FindLibqrcodegencpp
-------------------

FindModule for Libqrcodegencpp and associated libraries

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``Libqrcodegencpp::Libqrcodegencpp``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libqrcodegencpp_FOUND``
  True, if all required components and the core library were found.
``Libqrcodegencpp_VERSION``
  Detected version of found Libqrcodegencpp libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libqrcodegencpp_LIBRARY``
  Path to the library component of Libqrcodegencpp.
``Libqrcodegencpp_INCLUDE_DIR``
  Directory containing ``qrcodegen.hpp``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libqrcodegencpp QUIET qrcodegencpp)
endif()

# Libqrcodegencpp_set_soname: Set SONAME on imported library target
macro(Libqrcodegencpp_set_soname)
  if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
      COMMAND sh -c "otool -D '${Libqrcodegencpp_LIBRARY}' | grep -v '${Libqrcodegencpp_LIBRARY}'"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0 AND _output MATCHES "^@rpath/")
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_SONAME "${_output}")
    endif()
  elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    execute_process(
      COMMAND sh -c "${CMAKE_OBJDUMP} -p '${Libqrcodegencpp_LIBRARY}' | grep SONAME"
      OUTPUT_VARIABLE _output
      RESULT_VARIABLE _result)

    if(_result EQUAL 0)
      string(REGEX REPLACE "[ \t]+SONAME[ \t]+([^ \t]+)" "\\1" _soname "${_output}")
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_SONAME "${_soname}")
      unset(_soname)
    endif()
  endif()
  unset(_output)
  unset(_result)
endmacro()

# Libqrcodegencpp_find_dll: Find DLL for corresponding import library
macro(Libqrcodegencpp_find_dll)
  cmake_path(GET Libqrcodegencpp_IMPLIB PARENT_PATH _implib_path)
  cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")

  string(REGEX REPLACE "[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" _dll_version "${Libqrcodegencpp_VERSION}")

  find_program(
    Libqrcodegencpp_LIBRARY
    NAMES qrcodegencpp.dll
    HINTS ${_implib_path} ${_bin_path}
    DOC "Libqrcodegencpp DLL location")

  if(NOT Libqrcodegencpp_LIBRARY)
    set(Libqrcodegencpp_LIBRARY "${Libqrcodegencpp_IMPLIB}")
  endif()
  unset(_implib_path)
  unset(_bin_path)
  unset(_dll_version)
endmacro()

find_path(
  Libqrcodegencpp_INCLUDE_DIR
  NAMES qrcodegen.hpp
  HINTS ${PC_Libqrcodegencpp_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES qrcodegencpp qrcodegen
  DOC "Libqrcodegencpp include directory")

if(PC_Libqrcodegencpp_VERSION VERSION_GREATER 0)
  set(Libqrcodegencpp_VERSION ${PC_Libqrcodegencpp_VERSION})
else()
  if(NOT Libqrcodegencpp_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libqrcodegencpp version.")
  endif()
  set(Libqrcodegencpp_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  find_library(
    Libqrcodegencpp_IMPLIB
    NAMES libqrcodegencpp qrcodegencpp
    DOC "Libqrcodegencpp import library location")

  libqrcodegencpp_find_dll()
else()
  find_library(
    Libqrcodegencpp_LIBRARY
    NAMES libqrcodegencpp qrcodegencpp
    HINTS ${PC_Libqrcodegencpp_LIBRARY_DIRS}
    PATHS /usr/lib /usr/local/lib
    DOC "Libqrcodegencpp location")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Libqrcodegencpp_ERROR_REASON "Ensure that a qrcodegencpp distribution is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Libqrcodegencpp_ERROR_REASON "Ensure that qrcodegencpp is installed on the system.")
endif()

find_package_handle_standard_args(
  Libqrcodegencpp
  REQUIRED_VARS Libqrcodegencpp_LIBRARY Libqrcodegencpp_INCLUDE_DIR
  VERSION_VAR Libqrcodegencpp_VERSION REASON_FAILURE_MESSAGE "${Libqrcodegencpp_ERROR_REASON}")
mark_as_advanced(Libqrcodegencpp_INCLUDE_DIR Libqrcodegencpp_LIBRARY Libqrcodegencpp_IMPLIB)
unset(Libqrcodegencpp_ERROR_REASON)

if(Libqrcodegencpp_FOUND)
  if(NOT TARGET Libqrcodegencpp::Libqrcodegencpp)
    if(IS_ABSOLUTE "${Libqrcodegencpp_LIBRARY}")
      if(DEFINED Libqrcodegencpp_IMPLIB)
        if(Libqrcodegencpp_IMPLIB STREQUAL Libqrcodegencpp_LIBRARY)
          add_library(Libqrcodegencpp::Libqrcodegencpp STATIC IMPORTED)
        else()
          add_library(Libqrcodegencpp::Libqrcodegencpp SHARED IMPORTED)
          set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_IMPLIB "${Libqrcodegencpp_IMPLIB}")
        endif()
      else()
        add_library(Libqrcodegencpp::Libqrcodegencpp UNKNOWN IMPORTED)
      endif()
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_LOCATION "${Libqrcodegencpp_LIBRARY}")
    else()
      add_library(Libqrcodegencpp::Libqrcodegencpp INTERFACE IMPORTED)
      set_property(TARGET Libqrcodegencpp::Libqrcodegencpp PROPERTY IMPORTED_LIBNAME "${Libqrcodegencpp_LIBRARY}")
    endif()

    libqrcodegencpp_set_soname()
    set_target_properties(
      Libqrcodegencpp::Libqrcodegencpp
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Libqrcodegencpp_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Libqrcodegencpp_INCLUDE_DIR}"
                 VERSION ${Libqrcodegencpp_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libqrcodegencpp PROPERTIES
  URL "https://www.nayuki.io/page/qr-code-generator-library"
  DESCRIPTION "This project aims to be the best, clearest library for generating QR Codes in C++.")
