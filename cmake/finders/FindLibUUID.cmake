#[=======================================================================[.rst
FindLibUUID
-----------

FindModule for LibUUID and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``LibUUID::LibUUID``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``LibUUID_FOUND``
  True, if all required components and the core library were found.
``LibUUID_VERSION``
  Detected version of found LibUUID libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LibUUID_LIBRARY``
  Path to the library component of LibUUID.
``LibUUID_INCLUDE_DIR``
  Directory containing ``LibUUID.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_LibUUID QUIET LibUUID uuid)
endif()

find_path(
  LibUUID_INCLUDE_DIR
  NAMES uuid/uuid.h
  HINTS ${PC_LibUUID_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "LibUUID include directory")

find_library(
  LibUUID_LIBRARY
  NAMES uuid
  HINTS ${PC_LibUUID_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "LibUUID location")

if(PC_LibUUID_VERSION VERSION_GREATER 0)
  set(LibUUID_VERSION ${PC_LibUUID_VERSION})
else()
  if(NOT LibUUID_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find LibUUID version.")
  endif()
  set(LibUUID_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  LibUUID
  REQUIRED_VARS LibUUID_LIBRARY LibUUID_INCLUDE_DIR
  VERSION_VAR LibUUID_VERSION REASON_FAILURE_MESSAGE "Ensure that e2fsprogs is installed on the system.")
mark_as_advanced(LibUUID_INCLUDE_DIR LibUUID_LIBRARY)

if(LibUUID_FOUND)
  if(NOT TARGET LibUUID::LibUUID)
    if(IS_ABSOLUTE "${LibUUID_LIBRARY}")
      add_library(LibUUID::LibUUID UNKNOWN IMPORTED)
      set_property(TARGET LibUUID::LibUUID PROPERTY IMPORTED_LOCATION "${LibUUID_LIBRARY}")
    else()
      add_library(LibUUID::LibUUID INTERFACE IMPORTED)
      set_property(TARGET LibUUID::LibUUID PROPERTY IMPORTED_LIBNAME "${LibUUID_LIBRARY}")
    endif()

    set_target_properties(
      LibUUID::LibUUID
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_LibUUID_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LibUUID_INCLUDE_DIR}"
                 VERSION ${LibUUID_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  LibUUID PROPERTIES
  URL "http://e2fsprogs.sourceforge.net/"
  DESCRIPTION
    "The libuuid library is used to generate unique identifiers for objects that may be accessible beyond the local system."
)
