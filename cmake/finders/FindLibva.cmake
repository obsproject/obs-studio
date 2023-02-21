#[=======================================================================[.rst
FindLibva
---------

FindModule for Libva and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the following :prop_tgt:`IMPORTED` targets:

``Libva::va``
  Video Acceleration (VA) API for Linux -- runtime

``Libva::drm``
  Video Acceleration (VA) API for Linux -- DRM runtime

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libva_FOUND``
  True, if all required components and the core library were found.
``Libva_VERSION``
  Detected version of found Libva libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libva_LIBRARY``
  Path to the library component of Libva.
``Libva_INCLUDE_DIR``
  Directory containing ``libva.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libva QUIET libva)
  pkg_search_module(PC_LibvaDrm QUIET libva-drm)
endif()

find_path(
  Libva_INCLUDE_DIR
  NAMES va.h
  HINTS ${PC_Libva_INCLUDE_DIRS}
  PATHS /usr/include/ /usr/local/include
  PATH_SUFFIXES va
  DOC "Libva include directory")

find_library(
  Libva_LIBRARY
  NAMES libva va
  HINTS ${PC_Libva_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libva location")

find_library(
  LibvaDrm_LIBRARY
  NAMES libva-drm va-drm
  HINTS ${PC_LibvaDrm_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libva-drm location")

if(PC_Libva_VERSION VERSION_GREATER 0)
  set(Libva_VERSION ${PC_Libva_VERSION})
elseif(EXISTS "${Libva_INCLUDE_DIR}/va_version.h")
  file(STRINGS "${Libva_INCLUDE_DIR}/va_version.h" _VERSION_STRING
       REGEX "^.*(MAJOR|MINOR|MICRO)_VERSION[ \t]+[0-9]+[ \t]*$")
  string(REGEX REPLACE ".*MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" VERSION_MAJOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*MINOR_VERSION[ \t]+([0-9]+).*" "\\1" VERSION_MINOR "${_VERSION_STRING}")
  string(REGEX REPLACE ".*MICRO_VERSION[ \t]+([0-9]+).*" "\\1" VERSION_MICRO "${_VERSION_STRING}")

  set(Libva_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_MICRO}")
else()
  if(NOT Libva_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libva version.")
  endif()
  set(Libva_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libva
  REQUIRED_VARS Libva_LIBRARY Libva_INCLUDE_DIR
  VERSION_VAR Libva_VERSION REASON_FAILURE_MESSAGE "Ensure that libva is installed on the system.")
mark_as_advanced(Libva_INCLUDE_DIR Libva_LIBRARY)

if(Libva_FOUND)
  if(NOT TARGET Libva::va)
    if(IS_ABSOLUTE "${Libva_LIBRARY}")
      add_library(Libva::va UNKNOWN IMPORTED)
      set_property(TARGET Libva::va PROPERTY IMPORTED_LOCATION "${Libva_LIBRARY}")
    else()
      add_library(Libva::va INTERFACE IMPORTED)
      set_property(TARGET Libva::va PROPERTY IMPORTED_LIBNAME "${Libva_LIBRARY}")
    endif()

    set_target_properties(
      Libva::va
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Libva_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Libva_INCLUDE_DIR}"
                 VERSION ${Libva_VERSION})
  endif()

  if(LibvaDrm_LIBRARY)
    if(NOT TARGET Libva::drm)
      if(IS_ABSOLUTE "${LibvaDrm_LIBRARY}")
        add_library(Libva::drm UNKNOWN IMPORTED)
        set_property(TARGET Libva::drm PROPERTY IMPORTED_LOCATION "${LibvaDrm_LIBRARY}")
      else()
        add_library(Libva::drm INTERFACE IMPORTED)
        set_property(TARGET Libva::drm PROPERTY IMPORTED_LIBNAME "${LibvaDrm_LIBRARY}")
      endif()

      set_target_properties(
        Libva::drm
        PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_LibvaDrm_CFLAGS_OTHER}"
                   INTERFACE_INCLUDE_DIRECTORIES "${Libva_INCLUDE_DIR}"
                   VERSION ${Libva_VERSION})
    endif()
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libva PROPERTIES
  URL "https://01.org/intel-media-for-linux"
  DESCRIPTION
    "An implementation for VA-API (Video Acceleration API) - an open-source library which provides access to graphics hardware acceleration capabilities."
)
