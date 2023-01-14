# * Try to find libgbm. Once done this will define
#
# GBM_FOUND - system has ADW
# GBM_INCLUDE_DIRS - the ADW include directory
# GBM_LIBRARIES - the libraries needed to use ADW
# GBM_DEFINITIONS - Compiler switches required for using ADW

# Use pkg-config to get the directories and then use these values in the
# find_path() and find_library() calls

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_GBM gbm)
endif()

find_path(
  GBM_INCLUDE_DIR
  NAMES gbm.h
  HINTS ${_GBM_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include)

find_library(
  GBM_LIB
  NAMES ${_GBM_LIBRARIES} gbm
  HINTS ${_GBM_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GBM REQUIRED_VARS GBM_LIB GBM_INCLUDE_DIR)
mark_as_advanced(GBM_INCLUDE_DIR GBM_LIB)

if(GBM_FOUND)
  set(GBM_INCLUDE_DIRS ${GBM_INCLUDE_DIR})
  set(GBM_LIBRARIES ${GBM_LIB})

  if(NOT TARGET GBM::GBM)
    if(IS_ABSOLUTE "${GBM_LIBRARIES}")
      add_library(GBM::GBM UNKNOWN IMPORTED)
      set_target_properties(GBM::GBM PROPERTIES IMPORTED_LOCATION
                                                "${GBM_LIBRARIES}")
    else()
      add_library(GBM::GBM INTERFACE IMPORTED)
      set_target_properties(GBM::GBM PROPERTIES IMPORTED_LIBNAME
                                                "${GBM_LIBRARIES}")
    endif()

    set_target_properties(GBM::GBM PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                              "${_GBM_INCLUDE_DIRS}")
    target_link_libraries(GBM::GBM INTERFACE ${_GBM_LIBRARIES})
    target_compile_options(GBM::GBM INTERFACE ${_GBM_CFLAGS})
  endif()
endif()
