# Once done these will be defined:
#
#  ICONV_FOUND
#  ICONV_INCLUDE_DIRS
#  ICONV_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_ICONV QUIET iconv)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(ICONV_INCLUDE_DIR
	NAMES iconv.h
	HINTS
		ENV IconvPath${_lib_suffix}
		ENV IconvPath
		${_ICONV_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include)

find_library(ICONV_LIB
	NAMES ${_ICONV_LIBRARIES} iconv libiconv
	HINTS
		${_ICONV_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iconv DEFAULT_MSG ICONV_LIB ICONV_INCLUDE_DIR)
mark_as_advanced(ICONV_INCLUDE_DIR ICONV_LIB)

if(ICONV_FOUND)
	set(ICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR})
	set(ICONV_LIBRARIES ${ICONV_LIB})
endif()
