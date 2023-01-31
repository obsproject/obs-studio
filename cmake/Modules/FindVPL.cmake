# * Try to find libvpl
#
# Once done this will define
#
# VPL_FOUND - system has intel media sdk VPL_INCLUDE_DIRS - the intel media sdk include directory VPL_LIBRARIES - the
# libraries needed to use intel media sdk VPL_DEFINITIONS - Compiler switches required for using intel media sdk

# Use pkg-config to get the directories and then use these values in the find_path() and find_library() calls

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_VPL vpl)
endif()

find_path(
  VPL_INCLUDE_DIR
  NAMES mfxstructures.h
  HINTS ${_VPL_INCLUDE_DIRS} ${_VPL_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  PATH_SUFFIXES vpl/)

find_library(
  VPL_LIB
  NAMES ${_VPL_LIBRARIES} ${_VPL_LIBRARIES} vpl
  HINTS ${_VPL_LIBRARY_DIRS} ${_VPL_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VPL REQUIRED_VARS VPL_LIB VPL_INCLUDE_DIR)
mark_as_advanced(VPL_INCLUDE_DIR VPL_LIB)

if(VPL_FOUND)
  set(VPL_INCLUDE_DIRS ${VPL_INCLUDE_DIR})
  set(VPL_LIBRARIES ${VPL_LIB})

  if(NOT TARGET VPL::VPL)
    if(IS_ABSOLUTE "${VPL_LIBRARIES}")
      add_library(VPL::VPL UNKNOWN IMPORTED)
      set_target_properties(VPL::VPL PROPERTIES IMPORTED_LOCATION "${VPL_LIBRARIES}")
    else()
      add_library(VPL::VPL INTERFACE IMPORTED)
      set_target_properties(VPL::VPL PROPERTIES IMPORTED_LIBNAME "${VPL_LIBRARIES}")
    endif()

    set_target_properties(VPL::VPL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VPL_INCLUDE_DIRS}")
  endif()
endif()
