#[=======================================================================[.rst
FindLibv4l2
-----------

FindModule for Libv4l2 and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libv4l2::Libv4l2``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libv4l2_FOUND``
  True, if all required components and the core library were found.
``Libv4l2_VERSION``
  Detected version of found Libv4l2 libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libv4l2_LIBRARY``
  Path to the library component of Libv4l2.
``Libv4l2_INCLUDE_DIR``
  Directory containing ``libv4l2.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libv4l2 QUIET libv4l2 v4l2 v4l-utils)
endif()

find_path(
  Libv4l2_INCLUDE_DIR
  NAMES libv4l2.h
  HINTS ${PC_Libv4l2_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Libv4l2 include directory"
)

find_library(
  Libv4l2_LIBRARY
  NAMES v4l2
  HINTS ${PC_Libv4l2_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libv4l2 location"
)

if(PC_Libv4l2_VERSION VERSION_GREATER 0)
  set(Libv4l2_VERSION ${PC_Libv4l2_VERSION})
else()
  if(NOT Libv4l2_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libv4l2 version.")
  endif()
  set(Libv4l2_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libv4l2
  REQUIRED_VARS Libv4l2_LIBRARY Libv4l2_INCLUDE_DIR
  VERSION_VAR Libv4l2_VERSION
  REASON_FAILURE_MESSAGE "Ensure that v4l-utils is installed on the system."
)
mark_as_advanced(Libv4l2_INCLUDE_DIR Libv4l2_LIBRARY)

if(Libv4l2_FOUND)
  if(NOT TARGET Libv4l2::Libv4l2)
    if(IS_ABSOLUTE "${Libv4l2_LIBRARY}")
      add_library(Libv4l2::Libv4l2 UNKNOWN IMPORTED)
      set_property(TARGET Libv4l2::Libv4l2 PROPERTY IMPORTED_LOCATION "${Libv4l2_LIBRARY}")
    else()
      add_library(Libv4l2::Libv4l2 INTERFACE IMPORTED)
      set_property(TARGET Libv4l2::Libv4l2 PROPERTY IMPORTED_LIBNAME "${Libv4l2_LIBRARY}")
    endif()

    set_target_properties(
      Libv4l2::Libv4l2
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Libv4l2_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libv4l2_INCLUDE_DIR}"
        VERSION ${Libv4l2_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Lib4l2
  PROPERTIES
    URL "https://linuxtv.org/wiki/index.php/V4l-utils"
    DESCRIPTION "The v4l-utils are a series of packages for handling media devices."
)
