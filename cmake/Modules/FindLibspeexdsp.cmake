# Once done these will be defined:
#
#  LIBSPEEXDSP_FOUND
#  LIBSPEEXDSP_INCLUDE_DIRS
#  LIBSPEEXDSP_LIBRARIES
#
# For use in OBS:
#
#  SPEEXDSP_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_SPEEXDSP QUIET speexdsp libspeexdsp)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(SPEEXDSP_INCLUDE_DIR
	NAMES speex/speex_preprocess.h
	HINTS
		ENV speexPath${_lib_suffix}
		ENV speexPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${speexPath${_lib_suffix}}
		${speexPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_SPEEXDSP_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(SPEEXDSP_LIB
	NAMES ${_SPEEXDSP_LIBRARIES} speexdsp libspeexdsp
	HINTS
		ENV speexPath${_lib_suffix}
		ENV speexPath
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		${speexPath${_lib_suffix}}
		${speexPath}
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${_SPEEXDSP_LIBRARY_DIRS}
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
find_package_handle_standard_args(Libspeexdsp DEFAULT_MSG SPEEXDSP_LIB SPEEXDSP_INCLUDE_DIR)
mark_as_advanced(SPEEXDSP_INCLUDE_DIR SPEEXDSP_LIB)

if(LIBSPEEXDSP_FOUND)
	set(LIBSPEEXDSP_INCLUDE_DIRS ${SPEEXDSP_INCLUDE_DIR})
	set(LIBSPEEXDSP_LIBRARIES ${SPEEXDSP_LIB})
endif()
