#[=======================================================================[.rst
FindSIMDe
---------

FindModule for SIMD Everywhere

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``SIMDe::SIMDe``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``SIMDe_FOUND``
  True, if headers were found.
``SIMDe_VERSION``
  Detected version of found SIMDe Everywhere.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``SIMDe_INCLUDE_DIR``
  Directory containing ``simde/simde-common.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_SIMDe QUIET simde)
endif()

find_path(
  SIMDe_INCLUDE_DIR
  NAMES simde/simde-common.h
  HINTS ${PC_SIMDe_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "SIMD Everywhere include directory"
)

if(EXISTS "${SIMDe_INCLUDE_DIR}/simde/simde-common.h")
  file(
    STRINGS
    "${SIMDe_INCLUDE_DIR}/simde/simde-common.h"
    _version_string
    REGEX "^.*VERSION_(MAJOR|MINOR|MICRO)[ \t]+[0-9]+[ \t]*$"
  )

  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MICRO[ \t]+([0-9]+).*" "\\1" _version_release "${_version_string}")

  set(SIMDe_VERSION "${_version_major}.${_version_minor}.${_version_release}")
  unset(_version_major)
  unset(_version_minor)
  unset(_version_release)
else()
  if(NOT SIMDe_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find SIMD Everywhere version.")
  endif()
  set(SIMDe_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(SIMDe_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(SIMDe_ERROR_REASON "Ensure SIMD Everywhere is available in local library paths.")
endif()

find_package_handle_standard_args(
  SIMDe
  REQUIRED_VARS SIMDe_INCLUDE_DIR
  VERSION_VAR SIMDe_VERSION
  REASON_FAILURE_MESSAGE "${SIMDe_ERROR_REASON}"
)
mark_as_advanced(SIMDe_INCLUDE_DIR)
unset(SIMDe_ERROR_REASON)

if(SIMDe_FOUND)
  if(NOT TARGET SIMDe::SIMDe)
    add_library(SIMDe::SIMDe INTERFACE IMPORTED)
    set_target_properties(SIMDe::SIMDe PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMDe_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  SIMDe
  PROPERTIES
    URL "https://github.com/simd-everywhere/simde"
    DESCRIPTION "Implementations of SIMD instruction sets for systems which don't natively support them."
)
