#[=======================================================================[.rst
Findglib
--------

FindModule for glib and associated libraries

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the :prop_tgt:`IMPORTED` target ``glib::glib``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``glib_FOUND``
  True, if all required components and the core library were found.
``glib_VERSION``
  Detected version of found glib libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``glib_LIBRARY``
  Path to the library component of glib.
``glib_INCLUDE_DIR``
  Directory containing ``glib.h``

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_glib QUIET glib-2.0)
endif()

find_path(
  glib_INCLUDE_DIR
  NAMES glib.h
  HINTS ${PC_glig_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES glib-2.0
  DOC "glib include directory"
)

find_library(
  glib_LIBRARY
  NAMES libglib-2.0 glib-2.0
  HINTS ${PC_glib_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "glib-2.0 location"
)

find_path(glibconfig_INCLUDE_DIR NAMES glibconfig.h HINTS ${PC_glib_INCLUDE_DIRS} PATH_SUFFIXES glib-2.0/include)

if(PC_glib_VERSION VERSION_GREATER 0)
  set(glib_VERSION ${PC_glib_VERSION})
else()
  if(NOT glib_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find glib version.")
  endif()
  set(glib_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  glib
  REQUIRED_VARS glib_LIBRARY glib_INCLUDE_DIR glibconfig_INCLUDE_DIR
  VERSION_VAR glib_VERSION
  REASON_FAILURE_MESSAGE "Ensure that glib is installed on the system."
)
mark_as_advanced(gio_INCLUDE_DIR glib_INCLUDE_DIR glibconfig_INCLUDE_DIR)

if(glib_FOUND)
  if(NOT TARGET glib::glib)
    if(IS_ABSOLUTE "${glib_LIBRARY}")
      add_library(glib::glib UNKNOWN IMPORTED)
      set_property(TARGET glib::glib PROPERTY IMPORTED_LOCATION "${glib_LIBRARY}")
    else()
      add_library(glib::glib INTERFACE IMPORTED)
      set_property(TARGET glib::glib PROPERTY IMPORTED_LIBNAME "${glib_LIBRARY}")
    endif()

    set_target_properties(
      glib::glib
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_glib_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${glib_INCLUDE_DIR};${glibconfig_INCLUDE_DIR}"
        VERSION ${glib_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  glib
  PROPERTIES
    URL "https://docs.gtk.org/glib"
    DESCRIPTION
      "A library providing useful classes for general purpose I/O, networking, IPC, settings, and other high level application functionality.a general-purpose, portable utility library, which provides many useful data types, macros, type conversions, string utilities, file utilities, a mainloop abstraction, and so on."
)
