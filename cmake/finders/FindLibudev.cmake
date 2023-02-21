#[=======================================================================[.rst
FindLibudev
-----------

FindModule for Libudev and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libudev::Libudev``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libudev_FOUND``
  True, if all required components and the core library were found.
``Libudev_VERSION``
  Detected version of found Libudev libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libudev_LIBRARY``
  Path to the library component of Libudev.
``Libudev_INCLUDE_DIR``
  Directory containing ``libudev.h``.

#]=======================================================================]

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libudev QUIET libudev)
endif()

find_path(
  Libudev_INCLUDE_DIR
  NAMES libudev.h
  HINTS ${PC_Libudev_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  DOC "Libudev include directory")

find_library(
  Libudev_LIBRARY
  NAMES udev libudev
  HINTS ${PC_Libudev_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libudev location")

if(PC_Libudev_VERSION VERSION_GREATER 0)
  set(Libudev_VERSION ${PC_Libudev_VERSION})
else()
  if(NOT Libudev_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libudev version.")
  endif()
  set(Libudev_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libudev
  REQUIRED_VARS Libudev_LIBRARY Libudev_INCLUDE_DIR
  VERSION_VAR Libudev_VERSION REASON_FAILURE_MESSAGE "Ensure that Libudev is installed on the system.")
mark_as_advanced(Libudev_INCLUDE_DIR Libudev_LIBRARY)

if(Libudev_FOUND)
  if(NOT TARGET Libudev::Libudev)
    if(IS_ABSOLUTE "${Libudev_LIBRARY}")
      add_library(Libudev::Libudev UNKNOWN IMPORTED)
      set_property(TARGET Libudev::Libudev PROPERTY IMPORTED_LOCATION "${Libudev_LIBRARY}")
    else()
      add_library(Libudev::Libudev INTERFACE IMPORTED)
      set_property(TARGET Libudev::Libudev PROPERTY IMPORTED_LIBNAME "${Libudev_LIBRARY}")
    endif()

    set_target_properties(
      Libudev::Libudev
      PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_Libudev_CFLAGS_OTHER}"
                 INTERFACE_INCLUDE_DIRECTORIES "${Libudev_INCLUDE_DIR}"
                 VERSION ${Libudev_VERSION})
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libudev PROPERTIES
  URL "https://www.freedesktop.org/wiki/Software/systemd/"
  DESCRIPTION "API for enumerating and introspecting local devices.")
