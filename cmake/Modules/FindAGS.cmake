# Once done these will be defined:
#
#  AGS_FOUND
#  AGS_INCLUDE_DIRS
#  AGS_LIBRARIES
#
# For use in OBS: 
#
#  AGS_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_AGS QUIET ags)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
	set(_file_suffix 64)
else()
	set(_lib_suffix 32)
	set(_file_suffix 86)
endif()

find_path(AGS_INCLUDE_DIR
	NAMES amd_ags.h
	HINTS
		ENV agsPath${_lib_suffix}
		ENV agsPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${agsPath${_lib_suffix}}
		${agsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_AGS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(AGS_LIB
	NAMES ${_AGS_LIBRARIES} amd_ags_x${_file_suffix}
	HINTS
		ENV agsPath${_lib_suffix}
		ENV agsPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${agsPath${_lib_suffix}}
		${agsPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_AGS_LIBRARY_DIRS}
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
find_package_handle_standard_args(ags DEFAULT_MSG AGS_LIB AGS_INCLUDE_DIR)
mark_as_advanced(AGS_INCLUDE_DIR AGS_LIB)

if(AGS_FOUND)
	set(AGS_INCLUDE_DIRS ${AGS_INCLUDE_DIR})
	set(AGS_LIBRARIES ${AGS_LIB})
endif()
