# Once done these will be defined:
#
# LIBRIST_FOUND LIBRIST_INCLUDE_DIRS LIBRIST_LIBRARIES
#
# For use in OBS:
#
# LIBRIST_INCLUDE_DIR

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBRIST QUIET librist)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_lib_suffix 64)
else()
  set(_lib_suffix 32)
endif()

find_path(
  LIBRIST_INCLUDE_DIR
  NAMES librist.h librist/librist.h
  HINTS ENV LIBRIST_PATH ${LIBRIST_PATH} ${CMAKE_SOURCE_DIR}/${LIBRIST_PATH} ${_LIBRIST_INCLUDE_DIRS} ${DepsPath}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  PATH_SUFFIXES include)

find_library(
  LIBRIST_LIB
  NAMES ${_LIBRIST_LIBRARIES} librist rist
  HINTS ENV LIBRIST_PATH ${LIBRIST_PATH} ${CMAKE_SOURCE_DIR}/${LIBRIST_PATH} ${_LIBRIST_LIBRARY_DIRS} ${DepsPath}
  PATHS /usr/lib /usr/local/lib /opt/local/lib /sw/lib
  PATH_SUFFIXES
    lib${_lib_suffix}
    lib
    libs${_lib_suffix}
    libs
    bin${_lib_suffix}
    bin
    ../lib${_lib_suffix}
    ../lib
    ../libs${_lib_suffix}
    ../libs
    ../bin${_lib_suffix}
    ../bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Librist DEFAULT_MSG LIBRIST_LIB LIBRIST_INCLUDE_DIR)
mark_as_advanced(LIBRIST_INCLUDE_DIR LIBRIST_LIB)

if(LIBRIST_FOUND)
  set(LIBRIST_INCLUDE_DIRS ${LIBRIST_INCLUDE_DIR})
  set(LIBRIST_LIBRARIES ${LIBRIST_LIB})

  if(NOT TARGET Librist::Librist)
    if(IS_ABSOLUTE "${LIBRIST_LIBRARIES}")
      add_library(Librist::Librist UNKNOWN IMPORTED)
      set_target_properties(Librist::Librist PROPERTIES IMPORTED_LOCATION "${LIBRIST_LIBRARIES}")
    else()
      add_library(Librist::Librist INTERFACE IMPORTED)
      set_target_properties(Librist::Librist PROPERTIES IMPORTED_LIBNAME "${LIBRIST_LIBRARIES}")
    endif()

    set_target_properties(Librist::Librist PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBRIST_INCLUDE_DIRS}")
  endif()
else()
  message(STATUS "librist library not found")
endif()
