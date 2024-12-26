#[=======================================================================[.rst
FindPipeWire
------------

FindModule for PipeWire and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``PipeWire::PipeWire``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``PipeWire_FOUND``
  True, if all required components and the core library were found.
``PipeWire_VERSION``
  Detected version of found PipeWire libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``PipeWire_LIBRARY``
  Path to the library component of PipeWire.
``PipeWire_INCLUDE_DIR``
  Directory containing ``PipeWire.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_PipeWire libpipewire-0.3 QUIET)
  pkg_search_module(PC_Libspa libspa-0.2 QUIET)
endif()

find_path(
  PipeWire_INCLUDE_DIR
  NAMES pipewire/pipewire.h
  HINTS ${PC_PipeWire_INCLUDE_DIRS}
  PATH_SUFFIXES pipewire-0.3
  PATHS /usr/include /usr/local/include
  DOC "PipeWire include directory"
)

find_path(
  Libspa_INCLUDE_DIR
  NAMES spa/param/props.h
  HINTS ${PC_Libspa_INCLUDE_DIRS}
  PATH_SUFFIXES spa-0.2
  PATHS /usr/include /usr/local/include
  DOC "Libspa include directory"
)

find_library(
  PipeWire_LIBRARY
  NAMES pipewire-0.3
  HINTS ${PC_PipeWire_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "PipeWire location"
)

if(PC_PipeWire_VERSION VERSION_GREATER 0)
  set(PipeWire_VERSION ${PC_PipeWire_VERSION})
elseif(EXISTS "${PipeWire_INCLUDE_DIR}/pipewire/version.h")
  file(
    STRINGS
    "${PipeWire_INCLUDE_DIR}/pipewire/version.h"
    _version_string
    REGEX "^.*PW_(MAJOR|MINOR|MICRO)[ \t]+[0-9]+[ \t]*$"
  )
  string(REGEX REPLACE ".*PW_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*PW_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")
  string(REGEX REPLACE ".*PW_MICRO[ \t]+([0-9]+).*" "\\1" _version_micro "${_version_string}")

  set(PipeWire_VERSION "${_version_major}.${_version_minor}.${_version_micro}")
  unset(_version_major)
  unset(_version_minor)
  unset(_version_micro)
else()
  if(NOT PipeWire_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find PipeWire version.")
  endif()
  set(PipeWire_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  PipeWire
  REQUIRED_VARS PipeWire_LIBRARY PipeWire_INCLUDE_DIR Libspa_INCLUDE_DIR
  VERSION_VAR PipeWire_VERSION
  REASON_FAILURE_MESSAGE "Ensure that PipeWire is installed on the system."
)
mark_as_advanced(PipeWire_LIBRARY PipeWire_INCLUDE_DIR)

if(PipeWire_FOUND)
  if(NOT TARGET PipeWire::PipeWire)
    if(IS_ABSOLUTE "${PipeWire_LIBRARY}")
      add_library(PipeWire::PipeWire UNKNOWN IMPORTED)
      set_property(TARGET PipeWire::PipeWire PROPERTY IMPORTED_LOCATION "${PipeWire_LIBRARY}")
    else()
      add_library(PipeWire::PipeWire INTERFACE IMPORTED)
      set_property(TARGET PipeWire::PipeWire PROPERTY IMPORTED_LIBNAME "${PipeWire_LIBRARY}")
    endif()

    set_target_properties(
      PipeWire::PipeWire
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_PipeWire_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${PipeWire_INCLUDE_DIR};${Libspa_INCLUDE_DIR}"
        VERSION ${PipeWire_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  PipeWire
  PROPERTIES URL "https://www.pipewire.org" DESCRIPTION "PipeWire - multimedia processing"
)
