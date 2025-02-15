#[=======================================================================[.rst
FindXkbcommon
-------------

FindModule for xkbcommon and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``xkbcommon::xkbcommon``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Xkbcommon_FOUND``
  True, if all required components and the core library were found.
``Xkbcommon_VERSION``
  Detected version of found xkbcommon libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Xkbcommon_LIBRARY``
  Path to the library component of xkbcommon.
``Xkbcommon_INCLUDE_DIR``
  Directory containing ``xkbcommon.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Xkbcommon QUIET xkbcommon)
endif()

find_path(
  Xkbcommon_INCLUDE_DIR
  NAMES xkbcommon/xkbcommon.h
  HINTS ${PC_Xkbcommon_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES xkbcommon
  DOC "xkbcommon include directory"
)

find_library(
  Xkbcommon_LIBRARY
  NAMES xkbcommon libxkbcommon
  HINTS ${PC_Xkbcommon_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "xkbcommon location"
)

if(PC_Xkbcommon_VERSION VERSION_GREATER 0)
  set(Xkbcommon_VERSION ${PC_Xkbcommon_VERSION})
else()
  if(NOT Xkbcommon_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find xkbcommon version.")
  endif()
  set(Xkbcommon_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Xkbcommon
  REQUIRED_VARS Xkbcommon_LIBRARY Xkbcommon_INCLUDE_DIR
  VERSION_VAR Xkbcommon_VERSION
  REASON_FAILURE_MESSAGE "Ensure that xkbcommon is installed on the system."
)
mark_as_advanced(Xkbcommon_INCLUDE_DIR Xkbcommon_LIBRARY)

if(Xkbcommon_FOUND)
  if(NOT TARGET xkbcommon::xkbcommon)
    if(IS_ABSOLUTE "${Xkbcommon_LIBRARY}")
      add_library(xkbcommon::xkbcommon UNKNOWN IMPORTED)
      set_property(TARGET xkbcommon::xkbcommon PROPERTY IMPORTED_LOCATION "${Xkbcommon_LIBRARY}")
    else()
      add_library(xkbcommon::xkbcommon INTERFACE IMPORTED)
      set_property(TARGET xkbcommon::xkbcommon PROPERTY IMPORTED_LIBNAME "${Xkbcommon_LIBRARY}")
    endif()

    set_target_properties(
      xkbcommon::xkbcommon
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Xkbcommon_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Xkbcommon_INCLUDE_DIR}"
        VERSION ${Xkbcommon_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Xkbcommon
  PROPERTIES
    URL "https://www.xkbcommon.org"
    DESCRIPTION
      "A library for handling of keyboard descriptions, including loading them from disk, parsing them and handling their state."
)
