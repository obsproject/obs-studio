# * Try to find Libva, once done this will define
#
# * LIBVA_FOUND - system has Libva
# * LIBVA_INCLUDE_DIRS - the Libva include directory
# * LIBVA_LIBRARIES - the libraries needed to use Libva
# * LIBVA_DEFINITIONS - Compiler switches required for using Libva

# Use pkg-config to get the directories and then use these values in the
# find_path() and find_library() calls

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBVA libva)
endif()

find_path(
  LIBVA_INCLUDE_DIR
  NAMES va.h
  HINTS ${_LIBVA_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include
  PATH_SUFFIXES va/)

find_library(
  LIBVA_LIB
  NAMES ${_LIBVA_LIBRARIES} libva
  HINTS ${_LIBVA_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libva REQUIRED_VARS LIBVA_LIB
                                                      LIBVA_INCLUDE_DIR)
mark_as_advanced(LIBVA_INCLUDE_DIR LIBVA_LIB)

if(LIBVA_FOUND)
  set(LIBVA_INCLUDE_DIRS ${LIBVA_INCLUDE_DIR})
  set(LIBVA_LIBRARIES ${LIBVA_LIB})

  if(NOT TARGET Libva::va)
    if(IS_ABSOLUTE "${LIBVA_LIBRARIES}")
      add_library(Libva::va UNKNOWN IMPORTED)
      set_target_properties(Libva::va PROPERTIES IMPORTED_LOCATION
                                                 "${LIBVA_LIBRARIES}")
    else()
      add_library(Libva::va INTERFACE IMPORTED)
      set_target_properties(Libva::va PROPERTIES IMPORTED_LIBNAME
                                                 "${LIBVA_LIBRARIES}")
    endif()
  endif()
endif()
