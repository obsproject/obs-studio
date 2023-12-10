# Once done these will be defined:
#
# LIBDRM_FOUND LIBDRM_INCLUDE_DIRS LIBDRM_LIBRARIES

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBDRM QUIET libdrm)
endif()

find_path(
  LIBDRM_INCLUDE_DIR
  NAMES libdrm/drm_fourcc.h
  HINTS ${_LIBDRM_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include)

find_library(
  LIBDRM_LIB
  NAMES drm libdrm
  HINTS ${_LIBDRM_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libdrm DEFAULT_MSG LIBDRM_LIB LIBDRM_INCLUDE_DIR)
mark_as_advanced(LIBDRM_INCLUDE_DIR LIBDRM_LIB)

if(LIBDRM_FOUND)
  set(LIBDRM_INCLUDE_DIRS ${LIBDRM_INCLUDE_DIR})
  set(LIBDRM_LIBRARIES ${LIBDRM_LIB})

  if(NOT TARGET Libdrm::Libdrm)
    add_library(Libdrm::Libdrm INTERFACE IMPORTED)
    set_target_properties(Libdrm::Libdrm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INCLUDE_DIRS}")
  endif()
endif()
