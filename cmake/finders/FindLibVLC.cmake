#[=======================================================================[.rst
FindLibVLC
----------

FindModule for LibVLC and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``VLC::LibVLC``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``LibVLC_FOUND``
  True, if all required components and the core library were found.
``LibVLC_VERSION``
  Detected version of found LibVLC libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LibVLC_LIBRARY``
  Path to the library component of LibVLC.
``LibVLC_INCLUDE_DIR``
  Directory containing ``libvlc.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_LibVLC QUIET libvlc)
endif()

find_path(
  LibVLC_INCLUDE_DIR
  NAMES libvlc.h
  HINTS ${PC_LibVLC_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES vlc include/vlc include
  DOC "LibVLC include directory")

find_library(
  LibVLC_LIBRARY
  NAMES vlc libvlc
  HINTS ${PC_LibVLC_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "LibVLC location")

if(PC_LibVLC_VERSION VERSION_GREATER 0)
  set(LibVLC_VERSION ${PC_LibVLC_VERSION})
elseif(EXISTS "${LibVLC_INCLUDE_DIR}/libvlc_version.h")
  file(STRINGS "${LibVLC_INCLUDE_DIR}/libvlc_version.h" _VERSION_STRING
       REGEX "^.*LIBVLC_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+[ \t]*$")
  string(REGEX REPLACE ".*LIBVLC_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" VERSION_MAJOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*LIBVLC_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" VERSION_MINOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*LIBVLC_VERSION_REVISION[ \t]+([0-9]+).*" "\\1" VERSION_REVISION "${_VERSION_STRING}")
  set(LibVLC_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")
else()
  if(NOT LibVLC_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find LibVLC version.")
  endif()
  set(LibVLC_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  LibVLC
  REQUIRED_VARS LibVLC_LIBRARY LibVLC_INCLUDE_DIR
  VERSION_VAR LibVLC_VERSION REASON_FAILURE_MESSAGE "Ensure that libvlc-dev (vlc on BSD) is installed on the system.")
mark_as_advanced(LibVLC_INCLUDE_DIR LibVLC_LIBRARY)

if(LibVLC_FOUND)
  if(NOT TARGET VLC::LibVLC)
    if(IS_ABSOLUTE "${LibVLC_LIBRARY}")
      add_library(VLC::LibVLC UNKNOWN IMPORTED)
      set_property(TARGET VLC::LibVLC PROPERTY IMPORTED_LOCATION "${LibVLC_LIBRARY}")
    else()
      add_library(VLC::LibVLC INTERFACE IMPORTED)
      set_property(TARGET VLC::LibVLC PROPERTY IMPORTED_LIBNAME "${LibVLC_LIBRARY}")
    endif()

    set_target_properties(
      VLC::LibVLC
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_LibVLC_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LibVLC_INCLUDE_DIR}"
                 VERSION ${LibVLC_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  LibVLC PROPERTIES
  URL "https://www.videolan.org/vlc/libvlc.html"
  DESCRIPTION
    "libVLC is the core engine and the interface to the multimedia framework on which VLC media player is based.")
