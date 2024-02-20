#[=======================================================================[.rst
FindVPL
-------

FindModule for VPL and associated headers

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 3.0

This module defines the :prop_tgt:`IMPORTED` target ``VPL::VPL``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``VPL_FOUND``
  True, if headers were found.
``VPL_VERSION``
  Detected version of found VPL headers.
``VPL_LIBRARIES``
  Libraries needed to link to VPL.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``VPL_INCLUDE_DIR``
  Directory containing ``mfxdispatcher.h``.
``VPL_LIBRARY_RELEASE``
  Path to the library component of VPL in non-debug configuration.
``VPL_LIBRARY_DEBUG``
  Optional path to the library component of VPL in debug configuration.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-format: on

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(_VPL IMPORTED_TARGET QUIET vpl)
  if(_VPL_FOUND)
    add_library(VPL::VPL ALIAS PkgConfig::_VPL)
    return()
  endif()
endif()

find_path(
  VPL_INCLUDE_DIR
  NAMES vpl/mfxstructures.h
  HINTS ${_VPL_INCLUDE_DIRS} ${_VPL_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  DOC "VPL include directory")

find_library(
  VPL_LIBRARY_RELEASE
  NAMES ${_VPL_LIBRARIES} ${_VPL_LIBRARIES} vpl
  HINTS ${_VPL_LIBRARY_DIRS} ${_VPL_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  DOC "VPL library location")

find_library(
  VPL_LIBRARY_DEBUG
  NAMES ${_VPL_LIBRARIES} ${_VPL_LIBRARIES} vpld
  HINTS ${_VPL_LIBRARY_DIRS} ${_VPL_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  DOC "VPL debug library location")

include(SelectLibraryConfigurations)
select_library_configurations(VPL)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  VPL
  REQUIRED_VARS VPL_LIBRARY VPL_INCLUDE_DIR
  VERSION_VAR VPL_VERSION REASON_FAILURE_MESSAGE "${VPL_ERROR_REASON}")
mark_as_advanced(VPL_INCLUDE_DIR VPL_LIBRARY)
unset(VPL_ERROR_REASON)

if(EXISTS "${VPL_INCLUDE_DIR}/vpl/mfxdefs.h")
  file(STRINGS "${VPL_INCLUDE_DIR}/vpl/mfxdefs.h" _version_string REGEX "^.*VERSION_(MAJOR|MINOR)[ \t]+[0-9]+[ \t]*$")

  string(REGEX REPLACE ".*VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _version_major "${_version_string}")
  string(REGEX REPLACE ".*VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _version_minor "${_version_string}")

  set(VPL_VERSION "${_version_major}.${_version_minor}")
  unset(_version_major)
  unset(_version_minor)
else()
  if(NOT VPL_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find VPL version.")
  endif()
  set(VPL_VERSION 0.0.0)
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(VPL_ERROR_REASON "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(VPL_ERROR_REASON "Ensure VPL headers are available in local library paths.")
endif()

if(VPL_FOUND)
  set(VPL_INCLUDE_DIRS ${VPL_INCLUDE_DIR})
  set(VPL_LIBRARIES ${VPL_LIBRARY})

  if(NOT TARGET VPL::VPL)
    if(IS_ABSOLUTE "${VPL_LIBRARY_RELEASE}")
      add_library(VPL::VPL UNKNOWN IMPORTED)
      set_target_properties(VPL::VPL PROPERTIES IMPORTED_LOCATION "${VPL_LIBRARY_RELEASE}")
    else()
      add_library(VPL::VPL INTERFACE IMPORTED)
      set_target_properties(VPL::VPL PROPERTIES IMPORTED_LIBNAME "${VPL_LIBRARY_RELEASE}")
    endif()

    set_target_properties(
      VPL::VPL
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VPL_INCLUDE_DIRS}"
                 VERSION ${VPL_VERSION}
                 IMPORTED_CONFIGURATIONS Release)

    if(VPL_LIBRARY_DEBUG)
      if(IS_ABSOLUTE "${VPL_LIBRARY_DEBUG}")
        set_target_properties(VPL::VPL PROPERTIES IMPORTED_LOCATION_DEBUG "${VPL_LIBRARY_DEBUG}")
      else()
        set_target_properties(VPL::VPL PROPERTIES IMPORTED_LIBNAME_DEBUG "${VPL_LIBRARY_DEBUG}")
      endif()
      set_property(
        TARGET VPL::VPL
        APPEND
        PROPERTY IMPORTED_CONFIGURATIONS Debug)
    endif()
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  VPL PROPERTIES
  URL "https://github.com/oneapi-src/oneVPL"
  DESCRIPTION
    "Intel® oneAPI Video Processing Library (oneVPL) supports AI visual inference, media delivery, cloud gaming, and virtual desktop infrastructure use cases by providing access to hardware accelerated video decode, encode, and frame processing capabilities on Intel® GPUs."
)
