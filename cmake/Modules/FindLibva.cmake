# cmake-format: off
#
# This module defines the following variables:
#
# * LIBVA_FOUND - The component was found
# * LIBVA_INCLUDE_DIRS - The component include directory
# * LIBVA_LIBRARIES - The component library Libva
# * LIBVA_DRM_LIBRARIES - The component library Libva DRM

# Use pkg-config to get the directories and then use these values in the
# find_path() and find_library() calls
# cmake-format: on

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBVA libva)
  pkg_check_modules(_LIBVA_DRM libva-drm)
endif()

find_path(
  LIBVA_INCLUDE_DIR
  NAMES va/va.h va/va_drm.h
  HINTS ${_LIBVA_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include)

find_library(
  LIBVA_LIB
  NAMES ${_LIBVA_LIBRARIES} libva
  HINTS ${_LIBVA_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib)

find_library(
  LIBVA_DRM_LIB
  NAMES ${_LIBVA_DRM_LIBRARIES} libva-drm
  HINTS ${_LIBVA_DRM_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libva REQUIRED_VARS LIBVA_INCLUDE_DIR LIBVA_LIB LIBVA_DRM_LIB)
mark_as_advanced(LIBVA_INCLUDE_DIR LIBVA_LIB LIBVA_DRM_LIB)

if(LIBVA_FOUND)
  set(LIBVA_INCLUDE_DIRS ${LIBVA_INCLUDE_DIR})
  set(LIBVA_LIBRARIES ${LIBVA_LIB})
  set(LIBVA_DRM_LIBRARIES ${LIBVA_DRM_LIB})

  if(NOT TARGET Libva::va)
    if(IS_ABSOLUTE "${LIBVA_LIBRARIES}")
      add_library(Libva::va UNKNOWN IMPORTED)
      set_target_properties(Libva::va PROPERTIES IMPORTED_LOCATION "${LIBVA_LIBRARIES}")
    else()
      add_library(Libva::va INTERFACE IMPORTED)
      set_target_properties(Libva::va PROPERTIES IMPORTED_LIBNAME "${LIBVA_LIBRARIES}")
    endif()

    set_target_properties(Libva::va PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBVA_INCLUDE_DIRS}")
  endif()

  if(NOT TARGET Libva::drm)
    if(IS_ABSOLUTE "${LIBVA_DRM_LIBRARIES}")
      add_library(Libva::drm UNKNOWN IMPORTED)
      set_target_properties(Libva::drm PROPERTIES IMPORTED_LOCATION "${LIBVA_DRM_LIBRARIES}")
    else()
      add_library(Libva::drm INTERFACE IMPORTED)
      set_target_properties(Libva::drm PROPERTIES IMPORTED_LIBNAME "${LIBVA_DRM_LIBRARIES}")
    endif()

    set_target_properties(Libva::drm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBVA_INCLUDE_DIRS}")
  endif()

endif()
