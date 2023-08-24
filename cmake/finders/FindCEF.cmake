#[=======================================================================[.rst
FindCEF
----------

FindModule for CEF and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` targets:

``CEF::Wrapper``
  Static library loading wrapper

``CEF::Library``
  Chromium Embedded Library

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``CEF_FOUND``
  True, if all required components and the core library were found.
``CEF_VERSION``
  Detected version of found CEF libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``CEF_LIBRARY_WRAPPER_RELEASE``
  Path to the optimized wrapper component of CEF.
``CEF_LIBRARY_WRAPPER_DEBUG``
  Path to the debug wrapper component of CEF.
``CEF_LIBRARY_RELEASE``
  Path to the library component of CEF.
``CEF_LIBRARY_DEBUG``
  Path to the debug library component of CEF.
``CEF_INCLUDE_DIR``
  Directory containing ``cef_version.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0301
# cmake-lint: disable=C0307
# cmake-format: on

include(FindPackageHandleStandardArgs)

set(CEF_ROOT_DIR
    ""
    CACHE PATH "Alternative path to Chromium Embedded Framework")

if(NOT DEFINED CEF_ROOT_DIR OR CEF_ROOT_DIR STREQUAL "")
  message(
    FATAL_ERROR
      "CEF_ROOT_DIR is not set - if ENABLE_BROWSER is enabled, "
      "a CEF distribution with compiled wrapper library is required.\n"
      "Please download a CEF distribution for your appropriate architecture "
      "and specify CEF_ROOT_DIR to its location")
endif()

find_path(
  CEF_INCLUDE_DIR "cef_version.h"
  HINTS "${CEF_ROOT_DIR}/include"
  DOC "Chromium Embedded Framework include directory.")

if(CEF_INCLUDE_DIR)
  file(STRINGS "${CEF_INCLUDE_DIR}/cef_version.h" _VERSION_STRING
       REGEX "^.*CEF_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+[ \t]*$")
  string(REGEX REPLACE ".*CEF_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" VERSION_MAJOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*CEF_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" VERSION_MINOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*CEF_VERSION_PATCH[ \t]+([0-9]+).*" "\\1" VERSION_PATCH "${_VERSION_STRING}")
  set(CEF_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
else()
  if(NOT CEF_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Chromium Embedded Framework version.")
  endif()
  set(CEF_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  find_library(
    CEF_IMPLIB_RELEASE
    NAMES cef.lib libcef.lib
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}" "${CEF_ROOT_DIR}/Release"
    DOC "Chromium Embedded Framework import library location")

  find_program(
    CEF_LIBRARY_RELEASE
    NAMES cef.dll libcef.dll
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}" "${CEF_ROOT_DIR}/Release"
    DOC "Chromium Embedded Framework library location")

  if(NOT CEF_LIBRARY_RELEASE)
    set(CEF_LIBRARY_RELEASE "${CEF_IMPLIB_RELEASE}")
  endif()

  find_library(
    CEF_LIBRARY_WRAPPER_RELEASE
    NAMES cef_dll_wrapper libcef_dll_wrapper
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}/build/libcef_dll/Release" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper/Release"
          "${CEF_ROOT_DIR}/build/libcef_dll" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper"
    DOC "Chromium Embedded Framework static library wrapper.")

  find_library(
    CEF_LIBRARY_WRAPPER_DEBUG
    NAMES cef_dll_wrapper libcef_dll_wrapper
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}/build/libcef_dll/Debug" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper/Debug"
    DOC "Chromium Embedded Framework static library wrapper (debug).")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Darwin)
  find_library(
    CEF_LIBRARY_RELEASE
    NAMES "Chromium Embedded Framework"
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}" "${CEF_ROOT_DIR}/Release"
    DOC "Chromium Embedded Framework")

  find_library(
    CEF_LIBRARY_WRAPPER_RELEASE
    NAMES cef_dll_wrapper libcef_dll_wrapper
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}/build/libcef_dll/Release" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper/Release"
          "${CEF_ROOT_DIR}/build/libcef_dll" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper"
    DOC "Chromium Embedded Framework static library wrapper.")

  find_library(
    CEF_LIBRARY_WRAPPER_DEBUG
    NAMES cef_dll_wrapper libcef_dll_wrapper
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}/build/libcef_dll/Debug" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper/Debug"
    DOC "Chromium Embedded Framework static library wrapper (debug).")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  find_library(
    CEF_LIBRARY_RELEASE
    NAMES libcef.so
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}" "${CEF_ROOT_DIR}/Release"
    DOC "Chromium Embedded Framework")

  find_library(
    CEF_LIBRARY_WRAPPER_RELEASE
    NAMES cef_dll_wrapper.a libcef_dll_wrapper.a
    NO_DEFAULT_PATH
    PATHS "${CEF_ROOT_DIR}/build/libcef_dll" "${CEF_ROOT_DIR}/build/libcef_dll_wrapper"
    DOC "Chromium Embedded Framework static library wrapper.")
endif()

include(SelectLibraryConfigurations)
select_library_configurations(CEF)

find_package_handle_standard_args(
  CEF
  REQUIRED_VARS CEF_LIBRARY_RELEASE CEF_LIBRARY_WRAPPER_RELEASE CEF_INCLUDE_DIR
  VERSION_VAR CEF_VERSION REASON_FAILURE_MESSAGE
                          "Ensure that location of pre-compiled Chromium Embedded Framework is set as CEF_ROOT_DIR.")
mark_as_advanced(CEF_LIBRARY CEF_LIBRARY_WRAPPER_RELEASE CEF_LIBRARY_WRAPPER_DEBUG CEF_INCLUDE_DIR)

if(NOT TARGET CEF::Wrapper)
  if(IS_ABSOLUTE "${CEF_LIBRARY_WRAPPER_RELEASE}")
    add_library(CEF::Wrapper STATIC IMPORTED)
    set_property(TARGET CEF::Wrapper PROPERTY IMPORTED_LOCATION_RELEASE "${CEF_LIBRARY_WRAPPER_RELEASE}")
  else()
    add_library(CEF::Wrapper INTERFACE IMPORTED)
    set_property(TARGET CEF::Wrapper PROPERTY IMPORTED_LIBNAME_RELEASE "${CEF_LIBRARY_WRAPPER_RELEASE}")
  endif()
  set_property(
    TARGET CEF::Wrapper
    APPEND
    PROPERTY IMPORTED_CONFIGURATIONS "Release")

  if(CEF_LIBRARY_WRAPPER_DEBUG)
    if(IS_ABSOLUTE "${CEF_LIBRARY_WRAPPER_DEBUG}")
      set_property(TARGET CEF::Wrapper PROPERTY IMPORTED_LOCATION_DEBUG "${CEF_LIBRARY_WRAPPER_DEBUG}")
    else()
      set_property(TARGET CEF::Wrapper PROPERTY IMPORTED_LIBNAME_DEBUG "${CEF_LIBRARY_WRAPPER_DEBUG}")
    endif()
    set_property(
      TARGET CEF::Wrapper
      APPEND
      PROPERTY IMPORTED_CONFIGURATIONS "Debug")
  endif()

  set_property(
    TARGET CEF::Wrapper
    APPEND
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CEF_INCLUDE_DIR}" "${CEF_ROOT_DIR}")
endif()

if(NOT TARGET CEF::Library)
  if(IS_ABSOLUTE "${CEF_LIBRARY_RELEASE}")
    if(DEFINED CEF_IMPLIB_RELEASE)
      if(CEF_IMPLIB_RELEASE STREQUAL CEF_LIBRARY_RELEASE)
        add_library(CEF::Library STATIC IMPORTED)
      else()
        add_library(CEF::Library SHARED IMPORTED)
        set_property(TARGET CEF::Library PROPERTY IMPORTED_IMPLIB_RELEASE "${CEF_IMPLIB_RELEASE}")
      endif()
    else()
      add_library(CEF::Library UNKNOWN IMPORTED)
    endif()
    set_property(TARGET CEF::Library PROPERTY IMPORTED_LOCATION_RELEASE "${CEF_LIBRARY_RELEASE}")
  else()
    add_library(CEF::Library INTERFACE IMPORTED)
    set_property(TARGET CEF::Library PROPERTY IMPORTED_LIBNAME_RELEASE "${CEF_LIBRARY_RELEASE}")
  endif()

  set_property(
    TARGET CEF::Library
    APPEND
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CEF_INCLUDE_DIR}" "${CEF_ROOT_DIR}")
  set_property(TARGET CEF::Library PROPERTY IMPORTED_CONFIGURATIONS "Release")
endif()

include(FeatureSummary)
set_package_properties(
  CEF PROPERTIES
  URL "https://bitbucket.org/chromiumembedded/cef/"
  DESCRIPTION
    "Chromium Embedded Framework (CEF). A simple framework for embedding Chromium-based browsers in other applications."
)
