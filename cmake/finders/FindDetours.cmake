#[=======================================================================[.rst
FindDetours
-----------

FindModule for Detours and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Detours::Detours``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Detours_FOUND``
  True, if all required components and the core library were found.
``Detours_VERSION``
  Detected version of found Detours libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Detours_LIBRARY``
  Path to the library component of Detours.
``Detours_INCLUDE_DIR``
  Directory containing ``detours.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_Detours QUIET detours)
endif()

find_path(Detours_INCLUDE_DIR NAMES detours.h HINTS ${PC_Detours_INCLUDE_DIRS} DOC "Detours include directory")

find_library(Detours_IMPLIB NAMES detours HINTS ${PC_Detours_LIBRARY_DIRS} DOC "Detours location")

cmake_path(GET Detours_IMPLIB PARENT_PATH _implib_path)
cmake_path(SET _bin_path NORMALIZE "${_implib_path}/../bin")

find_program(Detours_LIBRARY NAMES detours.dll HINTS ${_implib_path} ${_bin_path} DOC "Detours DLL location")

if(NOT Detours_LIBRARY)
  set(Detours_LIBRARY "${Detours_IMPLIB}")
endif()
unset(_implib_path)
unset(_bin_path)

if(PC_Detours_VERSION VERSION_GREATER 0)
  set(Detours_VERSION ${PC_Detours_VERSION})
else()
  if(NOT Detours_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find detours version.")
  endif()
  set(Detours_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Detours
  REQUIRED_VARS Detours_LIBRARY Detours_INCLUDE_DIR
  VERSION_VAR Detours_VERSION
  REASON_FAILURE_MESSAGE "Ensure that obs-deps is provided as part of CMAKE_PREFIX_PATH."
)
mark_as_advanced(Detours_INCLUDE_DIR Detours_LIBRARY)

if(Detours_FOUND)
  if(NOT TARGET Detours::Detours)
    if(IS_ABSOLUTE "${Detours_LIBRARY}")
      if(DEFINED Detours_IMPLIB)
        if(Detours_IMPLIB STREQUAL Detours_LIBRARY)
          add_library(Detours::Detours STATIC IMPORTED)
        else()
          add_library(Detours::Detours SHARED IMPORTED)
          set_property(TARGET Detours::Detours PROPERTY IMPORTED_IMPLIB "${Detours_IMPLIB}")
        endif()
      else()
        add_library(Detours::Detours UNKNOWN IMPORTED)
      endif()

      set_property(TARGET Detours::Detours PROPERTY IMPORTED_LOCATION "${Detours_LIBRARY}")
    else()
      add_library(Detours::Detours INTERFACE IMPORTED)
      set_property(TARGET Detours::Detours PROPERTY IMPORTED_LIBNAME "${Detours_LIBRARY}")
    endif()

    set_target_properties(
      Detours::Detours
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Detours_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Detours_INCLUDE_DIR}"
        VERSION ${Detours_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Detours
  PROPERTIES
    URL "https://github.com/microsoft/detours"
    DESCRIPTION "Detours is a software package for monitoring and instrumenting API calls on Windows."
)
