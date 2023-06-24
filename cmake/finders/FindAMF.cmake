#[=======================================================================[.rst
FindAMF
-------

FindModule for AMF and associated headers

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``AMF::AMF``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``AMF_FOUND``
  True, if headers were found.
``AMF_VERSION``
  Detected version of found AMF headers.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``AMF_INCLUDE_DIR``
  Directory containing ``AMF/core/Factory.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_path(
  AMF_INCLUDE_DIR
  NAMES AMF/core/Factory.h
  PATHS /usr/include /usr/local/include
  DOC "AMF include directory")

if(EXISTS "${AMF_INCLUDE_DIR}/AMF/core/Version.h")
  file(STRINGS "${AMF_INCLUDE_DIR}/AMF/core/Version.h" _version_string
       REGEX "^.*VERSION_(MAJOR|MINOR|RELEASE|BUILD_NUM)[ \t]+[0-9]+[ \t]*$")

  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
  string(REGEX REPLACE ".*VERSION_RELEASE[ \t]+([0-9]+).*" "\\1" _version_release "${_version_string}")

  set(AMF_VERSION "${_version_major}.${_version_minor}.${_version_release}")
  unset(_version_major)
  unset(_version_minor)
  unset(_version_release)
else()
  if(NOT AMF_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find AMF version.")
  endif()
  set(AMF_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(AMF_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(AMF_ERROR_REASON "Ensure AMF headers are available in local library paths.")
endif()

find_package_handle_standard_args(
  AMF
  REQUIRED_VARS AMF_INCLUDE_DIR
  VERSION_VAR AMF_VERSION REASON_FAILURE_MESSAGE "${AMF_ERROR_REASON}")
mark_as_advanced(AMF_INCLUDE_DIR)
unset(AMF_ERROR_REASON)

if(AMF_FOUND)
  if(NOT TARGET AMF::AMF)
    add_library(AMF::AMF INTERFACE IMPORTED)
    set_target_properties(AMF::AMF PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${AMF_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  AMF PROPERTIES
  URL "https://github.com/GPUOpen-LibrariesAndSDKs/AMF"
  DESCRIPTION
    "AMF is a light-weight, portable multimedia framework that abstracts away most of the platform and API-specific details and allows for easy implementation of multimedia applications using a variety of technologies, such as DirectX 11, OpenGL, and OpenCL and facilitates an efficient interop between them."
)
