#[=======================================================================[.rst
FindOSS
-------

FindModule for OSS and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``OSS::OSS``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``OSS_FOUND``
  True, if all required components and the core library were found.
``OSS_VERSION``
  Detected version of found OSS libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``OSS_INCLUDE_DIR``
  Directory containing ``sys/soundcard.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_path(
  OSS_INCLUDE_DIR
  NAMES sys/soundcard.h
  HINTS ${PC_OSS_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "OSS include directory")

set(OSS_VERSION ${CMAKE_HOST_SYSTEM_VERSION})

find_package_handle_standard_args(
  OSS
  REQUIRED_VARS OSS_INCLUDE_DIR
  VERSION_VAR OSS_VERSION REASON_FAILURE_MESSAGE "Ensure that OSS is installed on the system.")
mark_as_advanced(OSS_INCLUDE_DIR OSS_LIBRARY)

if(OSS_FOUND)
  if(NOT TARGET OSS::OSS)
    add_library(OSS::OSS INTERFACE IMPORTED)

    set_target_properties(OSS::OSS PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OSS_INCLUDE_DIR}" VERSION ${OSS_VERSION})
  endif()
endif()
