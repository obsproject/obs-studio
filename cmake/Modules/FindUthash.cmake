#[=======================================================================[.rst
FindUthash
----------

FindModule for uthash and the associated library

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``Uthash::Uthash``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Uthash_FOUND``
  True, if the library was found.
``Uthash_VERSION``
  Detected version of found uthash library.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Uthash_INCLUDE_DIR``
  Directory containing ``uthash.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_path(
  Uthash_INCLUDE_DIR
  NAMES uthash.h
  PATHS /usr/include /usr/local/include
  DOC "uthash include directory")

if(EXISTS "${Uthash_INCLUDE_DIR}/uthash.h")
  file(STRINGS "${Uthash_INCLUDE_DIR}/uthash.h" _version_string
       REGEX "#define[ \t]+UTHASH_VERSION[ \t]+[0-9]+\\.[0-9]+\\.[0-9]+")

  string(REGEX REPLACE "#define[ \t]+UTHASH_VERSION[ \t]+([0-9]+\\.[0-9]+\\.[0-9]+)" "\\1" Uthash_VERSION
                       "${_version_string}")
else()
  if(NOT Uthash_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find uthash version.")
  endif()
  set(Uthash_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(Uthash_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(Uthash_ERROR_REASON "Ensure uthash library is available in local include paths.")
endif()

find_package_handle_standard_args(
  Uthash
  REQUIRED_VARS Uthash_INCLUDE_DIR
  VERSION_VAR Uthash_VERSION REASON_FAILURE_MESSAGE "${Uthash_ERROR_REASON}")
mark_as_advanced(Uthash_INCLUDE_DIR)
unset(Uthash_ERROR_REASON)

if(Uthash_FOUND)
  if(NOT TARGET Uthash::Uthash)
    add_library(Uthash::Uthash INTERFACE IMPORTED)
    set_target_properties(Uthash::Uthash PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Uthash_INCLUDE_DIR}")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Uthash PROPERTIES
  URL "https://troydhanson.github.io/uthash"
  DESCRIPTION "A hash table for C structures")
