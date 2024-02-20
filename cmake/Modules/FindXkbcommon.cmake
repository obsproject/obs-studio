# Once done these will be defined:
#
# XKBCOMMON_FOUND XKBCOMMON_INCLUDE_DIRS XKBCOMMON_LIBRARIES

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_XKBCOMMON QUIET xkbcommon)
endif()

find_path(
  XKBCOMMON_INCLUDE_DIR
  NAMES xkbcommon/xkbcommon.h
  HINTS ${_XKBCOMMON_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include)

find_library(
  XKBCOMMON_LIB
  NAMES xkbcommon libxkbcommon
  HINTS ${_XKBCOMMON_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Xkbcommon DEFAULT_MSG XKBCOMMON_LIB XKBCOMMON_INCLUDE_DIR)
mark_as_advanced(XKBCOMMON_INCLUDE_DIR XKBCOMMON_LIB)

if(XKBCOMMON_FOUND)
  set(XKBCOMMON_INCLUDE_DIRS ${XKBCOMMON_INCLUDE_DIR})
  set(XKBCOMMON_LIBRARIES ${XKBCOMMON_LIB})

  if(NOT TARGET Xkbcommon::Xkbcommon)
    if(IS_ABSOLUTE "${XKBCOMMON_LIBRARIES}")
      add_library(Xkbcommon::Xkbcommon UNKNOWN IMPORTED)
      set_target_properties(Xkbcommon::Xkbcommon PROPERTIES IMPORTED_LOCATION "${XKBCOMMON_LIBRARIES}")
    else()
      add_library(Xkbcommon::Xkbcommon INTERFACE IMPORTED)
      set_target_properties(Xkbcommon::Xkbcommon PROPERTIES IMPORTED_LIBNAME "${XKBCOMMON_LIBRARIES}")
    endif()

    set_target_properties(Xkbcommon::Xkbcommon PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${XKBCOMMON_INCLUDE_DIRS}")
  endif()
endif()
