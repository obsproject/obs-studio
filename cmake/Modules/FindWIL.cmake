#[=======================================================================[.rst
FindWIL
-------

FindModule for WIL and associated headers

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``WIL::WIL``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``WIL_FOUND``
  True, if headers were found.
``WIL_VERSION``
  Detected version of found WIL headers.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``WIL_INCLUDE_DIR``
  Directory containing ``cppwinrt.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(_WIL QUIET wil)
endif()

find_path(
  WIL_INCLUDE_DIR
  NAMES wil/cppwinrt.h
  HINTS ${_WI_INCLUDE_DIRS} ${_WIL_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  DOC "WIL include directory")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  WIL
  REQUIRED_VARS WIL_INCLUDE_DIR
  VERSION_VAR WIL_VERSION REASON_FAILURE_MESSAGE "${WIL_ERROR_REASON}")
mark_as_advanced(WIL_INCLUDE_DIR)
unset(WIL_ERROR_REASON)

if(EXISTS "${WIL_INCLUDE_DIR}/wil/cppwinrt.h")
  file(STRINGS "${WIL_INCLUDE_DIR}/wil/cppwinrt.h" _version_string REGEX "^.*VERSION_(MAJOR|MINOR)[ \t]+[0-9]+[ \t]*$")

  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")

  set(WIL_VERSION "${_version_major}.${_version_minor}")
  unset(_version_major)
  unset(_version_minor)
else()
  if(NOT WIL_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find WIL version.")
  endif()
  set(WIL_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(WIL_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(WIL_ERROR_REASON "Ensure WIL headers are available in local library paths.")
endif()

if(WIL_FOUND)
  set(WIL_INCLUDE_DIRS ${WIL_INCLUDE_DIR})

  if(NOT TARGET WIL::WIL)
    add_library(WIL::WIL INTERFACE IMPORTED)
    set_target_properties(WIL::WIL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${WIL_INCLUDE_DIRS}")

  endif()
endif()

include(FeatureSummary)
set_package_properties(
  WIL PROPERTIES
  URL "https://github.com/microsoft/wil.git"
  DESCRIPTION
    "The Windows Implementation Libraries (WIL) is a header-only C++ library created to make life easier for developers on Windows through readable type-safe C++ interfaces for common Windows coding patterns."
)
