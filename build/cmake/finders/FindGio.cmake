#[=======================================================================[.rst
FindGio
-------

FindModule for gio and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``gio::gio``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Gio_FOUND``
  True, if all required components and the core library were found.
``Gio_VERSION``
  Detected version of found gio libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Gio_LIBRARY``
  Path to the library component of gio.
``Gio_INCLUDE_DIR``
  Directory containing ``gio.h``

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Gio QUIET gio-2.0 gio)
  pkg_search_module(PC_GioUnix QUIET gio-unix-2.0)
endif()

find_path(
  Gio_INCLUDE_DIR
  NAMES gio/gio.h
  HINTS ${PC_Gio_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES glib-2.0
  DOC "gio-2.0 include directory"
)

find_path(
  GioUnix_INCLUDE_DIR
  NAMES gio/gunixfdmessage.h
  HINTS ${PC_GioUnix_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES gio-unix-2.0
  DOC "gio-unix-2.0 include directory"
)

find_library(
  Gio_LIBRARY
  NAMES libgio-2.0 gio-2.0 gio-unix-2.0
  HINTS ${PC_Gio_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "gio-2.0 location"
)

find_path(
  Glib_INCLUDE_DIR
  NAMES glibconfig.h
  HINTS ${PC_Gio_INCLUDE_DIRS}
  PATHS /usr/lib /usr/local/lib
  PATH_SUFFIXES glib-2.0/include
)

if(PC_Gio_VERSION VERSION_GREATER 0)
  set(Gio_VERSION ${PC_Gio_VERSION})
else()
  if(NOT Gio_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find gio version.")
  endif()
  set(Gio_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Gio
  REQUIRED_VARS Gio_LIBRARY Gio_INCLUDE_DIR GioUnix_INCLUDE_DIR Glib_INCLUDE_DIR
  VERSION_VAR Gio_VERSION
  REASON_FAILURE_MESSAGE "Ensure that glib is installed on the system."
)
mark_as_advanced(Gio_INCLUDE_DIR Gio_LIBRARY Glib_INCLUDE_DIR)

if(Gio_FOUND)
  if(NOT TARGET gio::gio)
    if(IS_ABSOLUTE "${Gio_LIBRARY}")
      add_library(gio::gio UNKNOWN IMPORTED)
      set_property(TARGET gio::gio PROPERTY IMPORTED_LOCATION "${Gio_LIBRARY}")
    else()
      add_library(gio::gio INTERFACE IMPORTED)
      set_property(TARGET gio::gio PROPERTY IMPORTED_LIBNAME "${Gio_LIBRARY}")
    endif()

    set_target_properties(
      gio::gio
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Gio_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Gio_INCLUDE_DIR};${GioUnix_INCLUDE_DIR};${Glib_INCLUDE_DIR}"
        VERSION ${Gio_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Gio
  PROPERTIES
    URL "https://docs.gtk.org/gio"
    DESCRIPTION
      "A library providing useful classes for general purpose I/O, networking, IPC, settings, and other high level application functionality."
)
