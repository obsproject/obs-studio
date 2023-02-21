#[=======================================================================[.rst
FindLibFDK
----------

FindModule for LibFDK and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``LibFDK::LibFDK``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``LibFDK_FOUND``
  True, if all required components and the core library were found.
``LibFDK_VERSION``
  Detected version of found LibFDK libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``LibFDK_LIBRARY``
  Path to the library component of libfdk.
``LibFDK_INCLUDE_DIR``
  Directory containing ``fdk-aac/aacenc_lib.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_LibFDK QUIET fdk-aac libfdk-aac)
endif()

find_path(
  LibFDK_INCLUDE_DIR
  NAMES fdk-aac/aacenc_lib.h
  HINTS ${PC_LibFDK_INCLUDE_DIRS}
  PATHS /usr/include/ /usr/local/include
  DOC "LibFDK include directory")

find_library(
  LibFDK_LIBRARY
  NAMES fdk-aac libfdk-aac
  HINTS ${PC_LibFDK_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "LibFDK location")

if(PC_LibFDK_VERSION VERSION_GREATER 0)
  set(LibFDK_VERSION ${PC_LibFDK_VERSION})
else()
  if(NOT LibFDK_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find LibFDK version.")
  endif()
  set(LibFDK_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  LibFDK
  REQUIRED_VARS LibFDK_LIBRARY LibFDK_INCLUDE_DIR
  VERSION_VAR LibFDK_VERSION REASON_FAILURE_MESSAGE "Ensure that libfdk is installed on the system.")
mark_as_advanced(LibFDK_INCLUDE_DIR LibFDK_LIBRARY)

if(LibFDK_FOUND)
  if(NOT TARGET LibFDK::LibFDK)
    if(IS_ABSOLUTE "${LibFDK_LIBRARY}")
      add_library(LibFDK::LibFDK UNKNOWN IMPORTED)
      set_property(TARGET LibFDK::LibFDK PROPERTY IMPORTED_LOCATION "${LibFDK_LIBRARY}")
    else()
      add_library(LibFDK::LibFDK INTERFACE IMPORTED)
      set_property(TARGET LibFDK::LibFDK PROPERTY IMPORTED_LIBNAME "${LibFDK_LIBRARY}")
    endif()

    set_target_properties(
      LibFDK::LibFDK
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_LibFDK_CFLAFGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LibFDK_INCLUDE_DIR}"
                 VERSION ${LibFDK_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  LibFDK PROPERTIES
  URL "https://github.com/mstorsjo/fdk-aac"
  DESCRIPTION "A standalone library of the Fraunhofer FDK AAC code from Android.")
