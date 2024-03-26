#[=======================================================================[.rst
FindLibpci
----------

FindModule for Libpci and associated libraries

.. versionchanged:: 3.0
  Updated FindModule to CMake standards

Imported Targets
^^^^^^^^^^^^^^^^

.. versionadded:: 2.0

This module defines the :prop_tgt:`IMPORTED` target ``Libpci::pci``.

Result Variables
^^^^^^^^^^^^^^^^

This module sets the following variables:

``Libpci_FOUND``
  True, if all required components and the core library were found.
``Libpci_VERSION``
  Detected version of found Libpci libraries.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Libpci_LIBRARY``
  Path to the library component of Libpci.
``Libpci_INCLUDE_DIR``
  Directory containing ``Libpci.h``.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PC_Libpci QUIET libpci)
endif()

find_path(
  Libpci_INCLUDE_DIR
  NAMES pci.h
  HINTS ${PC_Libpci_INCLUDE_DIRS}
  PATHS /usr/include/ /usr/local/include
  PATH_SUFFIXES pci
  DOC "Libpci include directory"
)

find_library(
  Libpci_LIBRARY
  NAMES libpci pci
  HINTS ${PC_Libpci_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib
  DOC "Libpci location"
)

if(PC_Libpci_VERSION VERSION_GREATER 0)
  set(Libpci_VERSION ${PC_Libpci_VERSION})
elseif(EXISTS "${Libpci_INCLUDE_DIR}/config.h")
  file(STRINGS "${Libpci_INCLUDE_DIR}/config.h" _VERSION_STRING REGEX "^.*PCILIB_VERSION[ \t]+\"[0-9\\.]+\"[ \t]*$")
  string(REGEX REPLACE ".*PCILIB_VERSION[ \t]+\"([0-9\\.]+)\".*" "\\1" Libpci_VERSION "${_VERSION_STRING}")
else()
  if(NOT Libpci_FIND_QUIETLY)
    message(AUTHOR_WARNING "Failed to find Libpci version.")
  endif()
  set(Libpci_VERSION 0.0.0)
endif()

find_package_handle_standard_args(
  Libpci
  REQUIRED_VARS Libpci_LIBRARY Libpci_INCLUDE_DIR
  VERSION_VAR Libpci_VERSION
  REASON_FAILURE_MESSAGE "Ensure that libpci is installed on the system."
)
mark_as_advanced(Libpci_INCLUDE_DIR Libpci_LIBRARY)

if(Libpci_FOUND)
  if(NOT TARGET Libpci::pci)
    if(IS_ABSOLUTE "${Libpci_LIBRARY}")
      add_library(Libpci::pci UNKNOWN IMPORTED)
      set_property(TARGET Libpci::pci PROPERTY IMPORTED_LOCATION "${Libpci_LIBRARY}")
    else()
      add_library(Libpci::pci INTERFACE IMPORTED)
      set_property(TARGET Libpci::pci PROPERTY IMPORTED_LIBNAME "${Libpci_LIBRARY}")
    endif()

    set_target_properties(
      Libpci::pci
      PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${PC_Libpci_CFLAFGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libpci_INCLUDE_DIR}"
        VERSION ${Libpci_VERSION}
    )
  endif()
endif()

include(FeatureSummary)
set_package_properties(
  Libpci
  PROPERTIES
    URL "https://mj.ucw.cz/sw/pciutils"
    DESCRIPTION "Offers access to the PCI configuration space on a variety of operating systems."
)
