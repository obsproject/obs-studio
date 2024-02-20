# cmake-format: off
# * Try to find Libpci Once done this will define
#
# * LIBPCI_FOUND - system has Libpci
# * LIBPCI_INCLUDE_DIRS - the Libpci include directory
# * LIBPCI_LIBRARIES - the libraries needed to use Libpci
# * LIBPCI_DEFINITIONS - Compiler switches required for using Libpci

# Use pkg-config to get the directories and then use these values in the
# find_path() and find_library() calls
# cmake-format: on

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBPCI libpci)
endif()

find_path(
  LIBPCI_INCLUDE_DIR
  NAMES pci.h
  HINTS ${_LIBPCI_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include
  PATH_SUFFIXES pci/)

find_library(
  LIBPCI_LIB
  NAMES ${_LIBPCI_LIBRARIES} libpci
  HINTS ${_LIBPCI_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libpci REQUIRED_VARS LIBPCI_LIB LIBPCI_INCLUDE_DIR)
mark_as_advanced(LIBPCI_INCLUDE_DIR LIBPCI_LIB)

if(LIBPCI_FOUND)
  set(LIBPCI_INCLUDE_DIRS ${LIBPCI_INCLUDE_DIR})
  set(LIBPCI_LIBRARIES ${LIBPCI_LIB})

  if(NOT TARGET LIBPCI::LIBPCI)
    if(IS_ABSOLUTE "${LIBPCI_LIBRARIES}")
      add_library(LIBPCI::LIBPCI UNKNOWN IMPORTED)
      set_target_properties(LIBPCI::LIBPCI PROPERTIES IMPORTED_LOCATION "${LIBPCI_LIBRARIES}")
    else()
      add_library(LIBPCI::LIBPCI INTERFACE IMPORTED)
      set_target_properties(LIBPCI::LIBPCI PROPERTIES IMPORTED_LIBNAME "${LIBPCI_LIBRARIES}")
    endif()
  endif()
endif()
