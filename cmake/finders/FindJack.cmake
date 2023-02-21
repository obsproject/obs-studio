#[=======================================================================[.rst
FindJack
--------

FindModule for Jack and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Jack::Jack``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Jack_FOUND``
  True, if all required components and the core library were found.
``Jack_VERSION``
  Detected version of found Jack libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Jack_LIBRARY``
  Path to the library component of Jack.
``Jack_INCLUDE_DIR``
  Directory containing ``jack.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Jack QUIET jack)
endif()

find_path(
  Jack_INCLUDE_DIR
  NAMES jack/jack.h
  HINTS ${PC_Jack_INCLUDE_DIR}
  PATHS /usr/include /usr/local/include
  DOC "Jack include directory")

find_library(
  Jack_LIBRARY
  NAMES jack
  HINTS ${PC_Jack_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Jack location")

if(PC_Jack_VERSION VERSION_GREATER 0)
  set(Jack_VERSION ${PC_Jack_VERSION})
else()
  if(NOT Jack_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Jack version.")
  endif()
  set(Jack_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Jack
  REQUIRED_VARS Jack_LIBRARY Jack_INCLUDE_DIR
  VERSION_VAR Jack_VERSION REASON_FAILURE_MESSAGE "Ensure that Jack is installed on the system.")
mark_as_advanced(Jack_INCLUDE_DIR Jack_LIBRARY)

if(Jack_FOUND)
  if(NOT TARGET Jack::Jack)
    if(IS_ABSOLUTE "${Jack_LIBRARY}")
      add_library(Jack::Jack UNKNOWN IMPORTED)
      set_property(TARGET Jack::Jack PROPERTY IMPORTED_LOCATION "${Jack_LIBRARY}")
    else()
      add_library(Jack::Jack INTERFACE IMPORTED)
      set_property(TARGET Jack::Jack PROPERTY IMPORTED_LIBNAME "${Jack_LIBRARY}")
    endif()

    set_target_properties(
      Jack::Jack
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Jack_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Jack_INCLUDE_DIR}"
                 VERSION ${Jack_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Jack PROPERTIES
  URL "https://www.jackaudio.org"
  DESCRIPTION
    "JACK Audio Connection Kit (or JACK) is a professional sound server API and pair of daemon implementations to provide real-time, low-latency connections for both audio and MIDI data between applications."
)
