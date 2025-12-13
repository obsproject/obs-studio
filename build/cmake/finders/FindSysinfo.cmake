#[=======================================================================[.rst
FindSysinfo
-----------

FindModule for Sysinfo and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Sysinfo::Sysinfo``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Sysinfo_FOUND``
  True, if all required components and the core library were found.
``Sysinfo_VERSION``
  Detected version of found Sysinfo libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Sysinfo_LIBRARY``
  Path to the library component of Sysinfo.
``Sysinfo_INCLUDE_DIR``
  Directory containing ``sys/sysinfo.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Sysinfo QUIET sysinfo)
endif()

find_path(
  Sysinfo_INCLUDE_DIR
  NAMES sys/sysinfo.h
  HINTS ${PC_Sysinfo_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Sysinfo include directory"
)

find_library(
  Sysinfo_LIBRARY
  NAMES sysinfo libsysinfo
  HINTS ${PC_Sysinfo_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Sysinfo location"
)

if(PC_Sysinfo_VERSION VERSION_GREATER 0)
  set(Sysinfo_VERSION ${PC_Sysinfo_VERSION})
else()
  if(NOT Sysinfo_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Sysinfo version.")
  endif()
  set(Sysinfo_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Sysinfo
  REQUIRED_VARS Sysinfo_LIBRARY Sysinfo_INCLUDE_DIR
  VERSION_VAR Sysinfo_VERSION
  REASON_FAILURE_MESSAGE "Ensure that Sysinfo is installed on the system."
)
mark_as_advanced(Sysinfo_INCLUDE_DIR Sysinfo_LIBRARY)

if(Sysinfo_FOUND)
  if(NOT TARGET Sysinfo::Sysinfo)
    if(IS_ABSOLUTE "${Sysinfo_LIBRARY}")
      add_library(Sysinfo::Sysinfo UNKNOWN IMPORTED)
      set_property(TARGET Sysinfo::Sysinfo PROPERTY IMPORTED_LOCATION "${Sysinfo_LIBRARY}")
    else()
      add_library(Sysinfo::Sysinfo INTERFACE IMPORTED)
      set_property(TARGET Sysinfo::Sysinfo PROPERTY IMPORTED_LIBNAME "${Sysinfo_LIBRARY}")
    endif()

    set_target_properties(
      Sysinfo::Sysinfo
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Sysinfo_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Sysinfo_INCLUDE_DIR}"
        VERSION ${Sysinfo_VERSION}
    )
  endif()
endif()
