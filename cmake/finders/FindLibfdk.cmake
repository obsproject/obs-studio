#[=======================================================================[.rst
FindLibfdk
----------

FindModule for Libfdk and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libfdk::Libfdk``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libfdk_FOUND``
  True, if all required components and the core library were found.
``Libfdk_VERSION``
  Detected version of found Libfdk libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libfdk_LIBRARY``
  Path to the library component of Libfdk.
``Libfdk_INCLUDE_DIR``
  Directory containing ``fdk-aac/aacenc_lib.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libfdk QUIET fdk-aac Libfdk-aac)
endif()

find_path(
  Libfdk_INCLUDE_DIR
  NAMES fdk-aac/aacenc_lib.h
  HINTS ${PC_Libfdk_INCLUDE_DIRS}
  PATHS /usr/include/ /usr/local/include
  DOC "Libfdk include directory"
)

find_library(
  Libfdk_LIBRARY
  NAMES fdk-aac Libfdk-aac
  HINTS ${PC_Libfdk_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libfdk location"
)

if(PC_Libfdk_VERSION VERSION_GREATER 0)
  set(Libfdk_VERSION ${PC_Libfdk_VERSION})
else()
  if(NOT Libfdk_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libfdk version.")
  endif()
  set(Libfdk_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libfdk
  REQUIRED_VARS Libfdk_LIBRARY Libfdk_INCLUDE_DIR
  VERSION_VAR Libfdk_VERSION
  REASON_FAILURE_MESSAGE "Ensure that Libfdk is installed on the system."
)
mark_as_advanced(Libfdk_INCLUDE_DIR Libfdk_LIBRARY)

if(Libfdk_FOUND)
  if(NOT TARGET Libfdk::Libfdk)
    if(IS_ABSOLUTE "${Libfdk_LIBRARY}")
      add_library(Libfdk::Libfdk UNKNOWN IMPORTED)
      set_property(TARGET Libfdk::Libfdk PROPERTY IMPORTED_LOCATION "${Libfdk_LIBRARY}")
    else()
      add_library(Libfdk::Libfdk INTERFACE IMPORTED)
      set_property(TARGET Libfdk::Libfdk PROPERTY IMPORTED_LIBNAME "${Libfdk_LIBRARY}")
    endif()

    set_target_properties(
      Libfdk::Libfdk
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Libfdk_CFLAFGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libfdk_INCLUDE_DIR}"
        VERSION ${Libfdk_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libfdk
  PROPERTIES
    URL "https://github.com/mstorsjo/fdk-aac"
    DESCRIPTION "A standalone library of the Fraunhofer FDK AAC code from Android."
)
