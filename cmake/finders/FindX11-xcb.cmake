#[=======================================================================[.rst
FindX11-xcb
-----------

FindModule for x11-xcb and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``X11::x11-xcb``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``X11-xcb_FOUND``
  True, if all required components and the core library were found.
``X11-xcb_VERSION``
  Detected version of found x11-xcb libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``X11-xcb_LIBRARY``
  Path to the library component of x11-xcb.
``X11-xcb_INCLUDE_DIR``
  Directory containing ``Xlib-xcb.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_X11-xcb QUIET x11-xcb)
endif()

find_path(
  X11-xcb_INCLUDE_DIR
  NAMES X11/Xlib-xcb.h
  HINTS ${PC_X11-xcb_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "X11-xcb include directory"
)

find_library(
  X11-xcb_LIBRARY
  NAMES X11-xcb
  HINTS ${PC_x11-xcb-LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "X11-xcb location"
)

if(PC_X11-xcb_VERSION VERSION_GREATER 0)
  set(X11-xcb_VERSION ${PC_X11-xcb_VERSION})
else()
  if(NOT X11-xcb_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find X11-xcb version.")
  endif()
  set(X11-xcb_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  X11-xcb
  REQUIRED_VARS X11-xcb_LIBRARY X11-xcb_INCLUDE_DIR
  VERSION_VAR X11-xcb_VERSION
  REASON_FAILURE_MESSAGE "Ensure that X11-xcb is installed on the system."
)
mark_as_advanced(X11-xcb_INCLUDE_DIR X11-xcb_LIBRARY)

if(X11-xcb_FOUND)
  if(NOT TARGET X11::x11-xcb)
    if(IS_ABSOLUTE "${X11-xcb_LIBRARY}")
      add_library(X11::x11-xcb UNKNOWN IMPORTED)
      set_property(TARGET X11::x11-xcb PROPERTY IMPORTED_LOCATION "${X11-xcb_LIBRARY}")
    else()
      add_library(X11::x11-xcb INTERFACE IMPORTED)
      set_property(TARGET X11::x11-xcb PROPERTY IMPORTED_LIBNAME "${X11-xcb_LIBRARY}")
    endif()

    set_target_properties(
      X11::x11-xcb
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_X11-xcb_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${X11-xcb_INCLUDE_DIR}"
        VERSION ${X11-xcb_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  X11-xcb
  PROPERTIES
    URL "https://www.X.org"
    DESCRIPTION
      "Provides functions needed by clients which take advantage of Xlib/XCB to mix calls to both Xlib and XCB over the same X connection."
)
