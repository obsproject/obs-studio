#[=======================================================================[.rst
FindSndio
---------

FindModule for Sndio and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Sndio::Sndio``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Sndio_FOUND``
  True, if all required components and the core library were found.
``Sndio_VERSION``
  Detected version of found Sndio libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Sndio_LIBRARY``
  Path to the library component of Sndio.
``Sndio_INCLUDE_DIR``
  Directory containing ``sndio.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Sndio QUIET sndio)
endif()

find_path(
  Sndio_INCLUDE_DIR
  NAMES sndio.h
  HINTS ${PC_Sndio_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Sndio include directory")

find_library(
  Sndio_LIBRARY
  NAMES sndio
  HINTS ${PC_Sndio_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Sndio location")

if(PC_Sndio_VERSION VERSION_GREATER 0)
  set(Sndio_VERSION ${PC_Sndio_VERSION})
else()
  if(NOT Sndio_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Sndio version.")
  endif()
  set(Sndio_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Sndio
  REQUIRED_VARS Sndio_LIBRARY Sndio_INCLUDE_DIR
  VERSION_VAR Sndio_VERSION REASON_FAILURE_MESSAGE "Ensure that Sndio is installed on the system.")
mark_as_advanced(Sndio_INCLUDE_DIR Sndio_LIBRARY)

if(Sndio_FOUND)
  if(NOT TARGET Sndio::Sndio)
    if(IS_ABSOLUTE "${Sndio_LIBRARY}")
      add_library(Sndio::Sndio UNKNOWN IMPORTED)
      set_property(TARGET Sndio::Sndio PROPERTY IMPORTED_LOCATION "${Sndio_LIBRARY}")
    else()
      add_library(Sndio::Sndio INTERFACE IMPORTED)
      set_property(TARGET Sndio::Sndio PROPERTY IMPORTED_LIBNAME "${Sndio_LIBRARY}")
    endif()

    set_target_properties(
      Sndio::Sndio
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Sndio_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Sndio_INCLUDE_DIR}"
                 VERSION ${Sndio_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Sndio PROPERTIES
  URL "https://www.sndio.org"
  DESCRIPTION "Sndio is a small audio and MIDI framework part of the OpenBSD project and ported to FreeBSD.")
