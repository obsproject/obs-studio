#[=======================================================================[.rst
FindLibAJANTV2
----------

FindModule for LibAJANTV2 and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``LibAJANTV2::LibAJANTV2``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``LibAJANTV2_FOUND``
  True, if all required components and the core library were found.
``LibAJANTV2_VERSION``
  Detected version of found LibAJANTV2 libraries.
``LibAJANTV2_INCLUDE_DIRS``
  Include directories needed for LibAJANTV2.
``LibAJANTV2_LIBRARIES``
  Libraries needed to link to LibAJANTV2.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LibAJANTV2_LIBRARY_RELEASE``
  Path to the library component of LibAJANTV2 in non-debug configuration.
``LibAJANTV2_LIBRARY_DEBUG``
  Optional path to the library component of LibAJANTV2 in debug configuration.
``LibAJANTV2_INCLUDE_DIR``
  Directory containing ``LibAJANTV2.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=C0301
# cmake-lint: disable=C0307
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_LibAJANTV2 QUIET ajantv2)
endif()

find_path(
  LibAJANTV2_INCLUDE_DIR
  NAMES ajalibraries
  HINTS ${PC_LibAJANTV2_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "LibAJANTV2 include directory")

find_library(
  LibAJANTV2_LIBRARY_RELEASE
  NAMES ajantv2 libajantv2
  HINTS ${PC_LibAJANTV2_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "LibAJANTV2 location")

find_library(
  LibAJANTV2_LIBRARY_DEBUG
  NAMES ajantv2d libajantv2d
  HINTS ${PC_LibAJANTV2_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "LibAJANTV2 debug location.")

if(PC_LibAJANTV2_VERSION VERSION_GREATER 0)
  set(LibAJANTV2_VERSION ${PC_LibAJANTV2_VERSION})
else()
  if(NOT LibAJANTV2_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find LibAJANTV2 version.")
  endif()
  set(LibAJANTV2_VERSION 0.0.0)
endif()

include(SelectLibraryConfigurations)
select_library_configurations(LibAJANTV2)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin|Windows")
  set(LibAJANTV2_ERROR_REASON "Ensure obs-deps is provided as part of CMAKE_PREFIX_PATH.")
elseif(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux|FreeBSD")
  set(LibAJANTV2_ERROR_REASON "Ensure ajantv2 static libraries are available in local library paths.")
endif()

find_package_handle_standard_args(
  LibAJANTV2
  REQUIRED_VARS LibAJANTV2_LIBRARY LibAJANTV2_INCLUDE_DIR
  VERSION_VAR LibAJANTV2_VERSION REASON_FAILURE_MESSAGE LibAJANTV2_ERROR_REASON)
mark_as_advanced(LibAJANTV2_LIBRARY LibAJANTV2_INCLUDE_DIR)
unset(LibAJANTV2_ERROR_REASON)

if(LibAJANTV2_FOUND)
  list(
    APPEND
    LibAJANTV2_INCLUDE_DIRS
    ${LibAJANTV2_INCLUDE_DIR}/ajalibraries
    ${LibAJANTV2_INCLUDE_DIR}/ajalibraries/ajaanc
    ${LibAJANTV2_INCLUDE_DIR}/ajalibraries/ajabase
    ${LibAJANTV2_INCLUDE_DIR}/ajalibraries/ajantv2
    ${LibAJANTV2_INCLUDE_DIR}/ajalibraries/ajantv2/includes)
  set(LibAJANTV2_LIBRARIES ${LibAJANTV2_LIBRARY})
  mark_as_advanced(LibAJANTV2_INCLUDE_DIR LibAJANTV2_LIBRARY)

  if(NOT TARGET AJA::LibAJANTV2)
    if(IS_ABSOLUTE "${LibAJANTV2_LIBRARY_RELEASE}")
      add_library(AJA::LibAJANTV2 STATIC IMPORTED)
      set_property(TARGET AJA::LibAJANTV2 PROPERTY IMPORTED_LOCATION "${LibAJANTV2_LIBRARY_RELEASE}")
    else()
      add_library(AJA::LibAJANTV2 INTERFACE IMPORTED)
      set_property(TARGET AJA::LibAJANTV2 PROPERTY IMPORTED_LIBNAME "${LibAJANTV2_LIBRARY_RELEASE}")
    endif()

    set_target_properties(
      AJA::LibAJANTV2
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_LibAJANTV2_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LibAJANTV2_INCLUDE_DIR}"
                 VERSION ${LibAJANTV2_VERSION}
                 IMPORTED_CONFIGURATIONS Release)

    if(LibAJANTV2_LIBRARY_DEBUG)
      if(IS_ABSOLUTE "${LibAJANTV2_LIBRARY_DEBUG}")
        set_property(TARGET AJA::LibAJANTV2 PROPERTY IMPORTED_LOCATION_DEBUG "${LibAJANTV2_LIBRARY_DEBUG}")
      else()
        set_property(TARGET AJA::LibAJANTV2 PROPERTY IMPORTED_LIBNAME_DEBUG "${LibAJANTV2_LIBRARY_DEBUG}")
      endif()
      set_property(
        TARGET AJA::LibAJANTV2
        APPEND
        PROPERTY IMPORTED_CONFIGURATIONS Debug)
    endif()

    set_target_properties(AJA::LibAJANTV2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LibAJANTV2_INCLUDE_DIRS}")
    set_property(
      TARGET AJA::LibAJANTV2
      APPEND
      PROPERTY INTERFACE_COMPILE_DEFINITIONS "$<$<BOOL:${OS_WINDOWS}>:AJA_WINDOWS;_WINDOWS;WIN32;MSWindows>"
               "$<$<AND:$<BOOL:${OS_WINDOWS}>,$<CONFIG:DEBUG>>:_DEBUG;_NDEBUG>" "$<$<BOOL:${OS_MACOS}>:AJAMac;AJA_MAC>"
               "$<$<BOOL:${OS_LINUX}>:AJA_LINUX;AJALinux>")
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  LibAJANTV2 PROPERTIES
  URL "https://www.aja.com"
  DESCRIPTION
    "AJA NTV2 SDK - AJA simplifies professional digital video workflows with a line of award-winning products designed and manufactured in Grass Valley, CA."
)
