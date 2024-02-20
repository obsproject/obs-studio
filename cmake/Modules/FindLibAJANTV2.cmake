# Once done these will be defined:
#
# LIBAJANTV2_FOUND LIBAJANTV2_INCLUDE_DIRS LIBAJANTV2_LIBRARIES
#
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(_AJA QUIET ajantv2)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(_lib_suffix 64)
else()
  set(_lib_suffix 32)
endif()

find_path(
  AJA_LIBRARIES_INCLUDE_DIR
  NAMES ajalibraries
  HINTS ENV
        AJASDKPath${_lib_suffix}
        ENV
        AJASDKPath
        ENV
        DepsPath${_lib_suffix}
        ENV
        DepsPath
        ${AJASDKPath${_lib_suffix}}
        ${AJASDKPath}
        ${DepsPath${_lib_suffix}}
        ${DepsPath}
        ${_AJA_NTV2_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /opt/local/include /sw/include
  PATH_SUFFIXES include)

find_library(
  AJA_NTV2_LIB
  NAMES ${_AJA_NTV2_LIBRARIES} ajantv2 libajantv2
  HINTS ENV
        AJASDKPath${_lib_suffix}
        ENV
        AJASDKPath
        ENV
        DepsPath${_lib_suffix}
        ENV
        DepsPath
        ${AJASDKPath${_lib_suffix}}
        ${AJASDKPath}
        ${DepsPath${_lib_suffix}}
        ${DepsPath}
        ${_AJA_NTV2_LIBRARY_DIRS}
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

find_library(
  AJA_NTV2_DEBUG_LIB
  NAMES ajantv2d libajantv2d
  HINTS ENV
        AJASDKPath${_lib_suffix}
        ENV
        AJASDKPath
        ENV
        DepsPath${_lib_suffix}
        ENV
        DepsPath
        ${AJASDKPath${_lib_suffix}}
        ${AJASDKPath}
        ${DepsPath${_lib_suffix}}
        ${DepsPath}
        ${_AJA_NTV2_LIBRARY_DIRS}
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
find_package_handle_standard_args(LibAJANTV2 DEFAULT_MSG AJA_LIBRARIES_INCLUDE_DIR AJA_NTV2_LIB)
mark_as_advanced(AJA_LIBRARIES_INCLUDE_DIR AJA_NTV2_LIB)

if(LIBAJANTV2_FOUND)
  set(AJA_LIBRARIES_INCLUDE_DIR ${AJA_LIBRARIES_INCLUDE_DIR}/ajalibraries)
  set(AJA_LIBRARIES_INCLUDE_DIRS
      ${AJA_LIBRARIES_INCLUDE_DIR} ${AJA_LIBRARIES_INCLUDE_DIR}/ajaanc ${AJA_LIBRARIES_INCLUDE_DIR}/ajabase
      ${AJA_LIBRARIES_INCLUDE_DIR}/ajantv2 ${AJA_LIBRARIES_INCLUDE_DIR}/ajantv2/includes)

  set(LIBAJANTV2_LIBRARIES ${AJA_NTV2_LIB})
  if(AJA_NTV2_DEBUG_LIB STREQUAL "AJA_NTV2_DEBUG_LIB-NOTFOUND")
    set(AJA_NTV2_DEBUG_LIB ${AJA_NTV2_LIB})
    mark_as_advanced(AJA_NTV2_DEBUG_LIB)
  endif()
  set(LIBAJANTV2_DEBUG_LIBRARIES ${AJA_NTV2_DEBUG_LIB})
  set(LIBAJANTV2_INCLUDE_DIRS ${AJA_LIBRARIES_INCLUDE_DIRS})

  if(NOT TARGET AJA::LibAJANTV2)
    if(IS_ABSOLUTE "${LIBAJANTV2_LIBRARIES}")
      add_library(AJA::LibAJANTV2 UNKNOWN IMPORTED)
      set_target_properties(AJA::LibAJANTV2 PROPERTIES IMPORTED_LOCATION "${LIBAJANTV2_LIBRARIES}")

      if(DEFINED LIBAJANTV2_DEBUG_LIBRARIES)
        set_target_properties(AJA::LibAJANTV2 PROPERTIES IMPORTED_LOCATION_DEBUG "${LIBAJANTV2_DEBUG_LIBRARIES}")
      endif()
    else()
      add_library(AJA::LibAJANTV2 INTERFACE IMPORTED)
      set_target_properties(AJA::LibAJANTV2 PROPERTIES IMPORTED_LIBNAME "${LIBAJANTV2_LIBRARIES}")

      if(DEFINED LIBAJANTV2_DEBUG_LIBRARIES)
        set_target_properties(AJA::LibAJANTV2 PROPERTIES IMPORTED_LIBNAME_DEBUG "${LIBAJANTV2_DEBUG_LIBRARIES}")
      endif()
    endif()

    set_target_properties(AJA::LibAJANTV2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBAJANTV2_INCLUDE_DIRS}")

    target_compile_definitions(
      AJA::LibAJANTV2
      INTERFACE "$<$<BOOL:${OS_WINDOWS}>:AJA_WINDOWS;_WINDOWS;WIN32;MSWindows>"
                "$<$<AND:$<BOOL:${OS_WINDOWS}>,$<CONFIG:DEBUG>>:_DEBUG;_NDEBUG>"
                "$<$<BOOL:${OS_MACOS}>:AJAMac;AJA_MAC>" "$<$<BOOL:${OS_LINUX}>:AJA_LINUX;AJALinux>")
  endif()
endif()
