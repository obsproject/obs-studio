#[=======================================================================[.rst
FindLibdrm
----------

FindModule for Libdrm and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libdrm::Libdrm``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libdrm_FOUND``
  True, if all required components and the core library were found.
``Libdrm_VERSION``
  Detected version of found Libdrm libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libdrm_LIBRARY``
  Path to the library component of Libdrm.
``Libdrm_INCLUDE_DIR``
  Directory containing ``drm_fourcc.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libdrm QUIET libdrm)
endif()

find_path(
  Libdrm_INCLUDE_DIR
  NAMES drm_fourcc.h
  HINTS ${PC_Libdrm_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES libdrm
  DOC "Libdrm include directory"
)

find_library(
  Libdrm_LIBRARY
  NAMES drm libdrm
  HINTS ${PC_Libdrm_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libdrm location"
)

if(PC_Libdrm_VERSION VERSION_GREATER 0)
  set(Libdrm_VERSION ${PC_Libdrm_VERSION})
else()
  if(NOT Libdrm_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libdrm version.")
  endif()
  set(Libdrm_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libdrm
  REQUIRED_VARS Libdrm_LIBRARY Libdrm_INCLUDE_DIR
  VERSION_VAR Libdrm_VERSION
  REASON_FAILURE_MESSAGE "Ensure that libdrm is installed on the system."
)
mark_as_advanced(Libdrm_INCLUDE_DIR Libdrm_LIBRARY)

if(Libdrm_FOUND)
  if(NOT TARGET Libdrm::Libdrm)
    if(IS_ABSOLUTE "${Libdrm_LIBRARY}")
      add_library(Libdrm::Libdrm UNKNOWN IMPORTED)
      set_property(TARGET Libdrm::Libdrm PROPERTY IMPORTED_LOCATION "${Libdrm_LIBRARY}")
    else()
      add_library(Libdrm::Libdrm INTERFACE IMPORTED)
      set_property(TARGET Libdrm::Libdrm PROPERTY IMPORTED_LIBNAME "${Libdrm_LIBRARY}")
    endif()

    set_target_properties(
      Libdrm::Libdrm
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Libdrm_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libdrm_INCLUDE_DIR}"
        VERSION ${Libdrm_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libdrm
  PROPERTIES
    URL "https://gitlab.freedesktop.org/mesa/drm"
    DESCRIPTION
      "A low-level library, typically used by graphics drivers such as the Mesa drivers, the X drivers, libva and similar projects."
)
