#[=======================================================================[.rst
FindSIMDE
---------

FindModule for SIMD Everywhere

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``SIMDE::SIMDE``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``SIMDE_FOUND``
  True, if headers were found.
``SIMDE_VERSION``
  Detected version of found SIMDE Everywhere.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``SIMDE_INCLUDE_DIR``
  Directory containing ``simde/simde-common.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_SIMDE QUIET simde)
endif()

find_path(
  SIMDE_INCLUDE_DIR
  NAMES simde/simde-common.h
  HINTS ${PC_SIMDE_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "SIMD Everywhere include directory")

if(EXISTS "${SIMDE_INCLUDE_DIR}/simde/simde-common.h")
  file(STRINGS "${SIMDE_INCLUDE_DIR}/simde/simde-common.h" _version_string
       REGEX "^.*VERSION_(MAJOR|MINOR|MICRO)[ \t]+[0-9]+[ \t]*$")

  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MICRO[ \t]+([0-9]+).*" "\\1" _version_release "${_version_string}")

  set(SIMDE_VERSION "${_version_major}.${_version_minor}.${_version_release}")
  unset(_version_major)
  unset(_version_minor)
  unset(_version_release)
else()
  if(NOT SIMDE_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find SIMD Everywhere version.")
  endif()
  set(SIMDE_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(SIMDE_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(SIMDE_ERROR_REASON "Ensure SIMD Everywhere is available in local library paths.")
endif()

find_package_handle_standard_args(
  SIMDE
  REQUIRED_VARS SIMDE_INCLUDE_DIR
  VERSION_VAR SIMDE_VERSION REASON_FAILURE_MESSAGE "${SIMDE_ERROR_REASON}")
mark_as_advanced(SIMDE_INCLUDE_DIR)
unset(SIMDE_ERROR_REASON)

if(SIMDE_FOUND)
  if(NOT TARGET SIMDE::SIMDE)
    add_library(SIMDE::SIMDE INTERFACE IMPORTED)
    set_target_properties(SIMDE::SIMDE PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SIMDE_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  SIMDE PROPERTIES
  URL "https://github.com/simd-everywhere/simde"
  DESCRIPTION "Implementations of SIMD instruction sets for systems which don't natively support them. ")
