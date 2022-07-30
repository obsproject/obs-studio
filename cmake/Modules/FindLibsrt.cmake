# Once done these will be defined:
#
# LIBSRT_FOUND LIBSRT_INCLUDE_DIRS LIBSRT_LIBRARIES
#
# For use in OBS:
#
# LIBSRT_INCLUDE_DIR

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_LIBSRT QUIET libsrt)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_lib_suffix 64)
else()
  set(_lib_suffix 32)
endif()

find_path(
  LIBSRT_INCLUDE_DIR
  NAMES srt.h srt/srt.h
  HINTS ENV LIBSRT_PATH ${LIBSRT_PATH} ${CMAKE_SOURCE_DIR}/${LIBSRT_PATH}
        ${_LIBSRT_INCLUDE_DIRS} ${DepsPath}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  PATH_SUFFIXES include)

find_library(
  LIBSRT_LIB
  NAMES ${_LIBSRT_LIBRARIES} srt libsrt
  HINTS ENV LIBSRT_PATH ${LIBSRT_PATH} ${CMAKE_SOURCE_DIR}/${LIBSRT_PATH}
        ${_LIBSRT_LIBRARY_DIRS} ${DepsPath}
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
find_package_handle_standard_args(Libsrt DEFAULT_MSG LIBSRT_LIB
                                  LIBSRT_INCLUDE_DIR)
mark_as_advanced(LIBSRT_INCLUDE_DIR LIBSRT_LIB)

if(LIBSRT_FOUND)
  set(LIBSRT_INCLUDE_DIRS ${LIBSRT_INCLUDE_DIR})
  set(LIBSRT_LIBRARIES ${LIBSRT_LIB})

  if(NOT TARGET Libsrt::Libsrt)
    if(IS_ABSOLUTE "${LIBSRT_LIBRARIES}")
      add_library(Libsrt::Libsrt UNKNOWN IMPORTED)
      set_target_properties(Libsrt::Libsrt PROPERTIES IMPORTED_LOCATION
                                                      "${LIBSRT_LIBRARIES}")
    else()
      add_library(Libsrt::Libsrt INTERFACE IMPORTED)
      set_target_properties(Libsrt::Libsrt PROPERTIES IMPORTED_LIBNAME
                                                      "${LIBSRT_LIBRARIES}")
    endif()

    set_target_properties(
      Libsrt::Libsrt PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                "${LIBSRT_INCLUDE_DIRS}")
  endif()
else()
  message(STATUS "libsrt library not found")
endif()
