# Once done these will be defined:
#
#  DETOURS_FOUND
#  DETOURS_INCLUDE_DIRS
#  DETOURS_LIBRARIES
#
# For use in OBS:
#
#  DETOURS_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_DETOURS QUIET detours)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(DETOURS_INCLUDE_DIR
	NAMES detours.h
	HINTS
		ENV detoursPath${_lib_suffix}
		ENV detoursPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${detoursPath${_lib_suffix}}
		${detoursPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_DETOURS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(DETOURS_LIB
	NAMES ${_DETOURS_LIBRARIES} detours
	HINTS
		ENV detoursPath${_lib_suffix}
		ENV detoursPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${detoursPath${_lib_suffix}}
		${detoursPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_DETOURS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Detours DEFAULT_MSG DETOURS_LIB DETOURS_INCLUDE_DIR)
mark_as_advanced(DETOURS_INCLUDE_DIR DETOURS_LIB)

if(DETOURS_FOUND)
	set(DETOURS_INCLUDE_DIRS ${DETOURS_INCLUDE_DIR})
	set(DETOURS_LIBRARIES ${DETOURS_LIB})
endif()
