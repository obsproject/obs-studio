#[=======================================================================[.rst
FindFFnvcodec
-------------

FindModule for FFnvcodec and the associated headers

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``FFnvcodec::FFnvcodec``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``FFnvcodec_FOUND``
  True, if headers was found.
``FFnvcodec_VERSION``
  Detected version of found FFnvcodec headers.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``FFnvcodec_INCLUDE_DIR``
  Directory containing ``nvEncodeAPI.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_FFnvcodec QUIET ffnvcodec)
endif()

find_path(
  FFnvcodec_INCLUDE_DIR
  NAMES ffnvcodec/nvEncodeAPI.h
  HINTS ${PC_FFnvcodec_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "FFnvcodec include directory"
)

if(PC_FFnvcodec_VERSION VERSION_GREATER 0)
  set(FFnvcodec_VERSION ${PC_FFnvcodec_VERSION})
elseif(EXISTS "${FFnvcodec_INCLUDE_DIR}/ffnvcodec/nvEncodeAPI.h")
  file(
    STRINGS
    "${FFnvcodec_INCLUDE_DIR}/ffnvcodec/nvEncodeAPI.h"
    _version_string
    REGEX "^.*NVENCAPI_(MAJOR|MINOR)_VERSION[ \t]+[0-9]+[ \t]*$"
  )

  string(REGEX REPLACE ".*MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*MINOR_VERSION[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")

  set(FFnvcodec_VERSION "${_version_major}.${_version_minor}")
  unset(_version_major)
  unset(_version_minor)
else()
  if(NOT FFnvcodec_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find FFnvcodec version.")
  endif()
  set(FFnvcodec_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(FFnvcodec_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(FFnvcodec_ERROR_REASON "Ensure FFnvcodec headers are available in local include paths.")
endif()

find_package_handle_standard_args(
  FFnvcodec
  REQUIRED_VARS FFnvcodec_INCLUDE_DIR
  VERSION_VAR FFnvcodec_VERSION
  HANDLE_VERSION_RANGE
  REASON_FAILURE_MESSAGE "${FFnvcodec_ERROR_REASON}"
)
mark_as_advanced(FFnvcodec_INCLUDE_DIR)
unset(FFnvcodec_ERROR_REASON)

if(FFnvcodec_FOUND)
  if(NOT TARGET FFnvcodec::FFnvcodec)
    add_library(FFnvcodec::FFnvcodec INTERFACE IMPORTED)
    set_target_properties(FFnvcodec::FFnvcodec PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FFnvcodec_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  FFnvcodec
  PROPERTIES
    URL "https://github.com/FFmpeg/nv-codec-headers/"
    DESCRIPTION "FFmpeg version of headers required to interface with NVIDIA's codec APIs."
)
