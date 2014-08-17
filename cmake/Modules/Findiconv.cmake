# Once done these will be defined:
#
#  LIBICONV_FOUND
#  LIBICONV_INCLUDE_DIRS
#  LIBICONV_LIBRARIES
#
# For use in OBS: 
#
#  ICONV_INCLUDE_DIR
#

if(LIBICONV_INCLUDE_DIRS AND LIBICONV_LIBRARIES)
	set(LIBICONV_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_ICONV QUIET iconv)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	set(ICONV_PATH_ARCH IConvPath${_lib_suffix})

	find_path(ICONV_INCLUDE_DIR
		NAMES iconv.h
		HINTS
			${_ICONV_INCLUDE_DIRS}
			"${CMAKE_SOURCE_DIR}/additional_install_files/include"
			"$ENV{obsAdditionalInstallFiles}/include"
			ENV IConvPath
			ENV ${ICONV_PATH_ARCH}
		PATHS
			/usr/include /usr/local/include /opt/local/include /sw/include)

	find_library(ICONV_LIB
		NAMES ${_ICONV_LIBRARIES} iconv libiconv
		HINTS
			${_ICONV_LIBRARY_DIRS}
			"${ICONV_INCLUDE_DIR}/../lib"
			"${ICONV_INCLUDE_DIR}/../lib${_lib_suffix}"
			"${ICONV_INCLUDE_DIR}/../libs${_lib_suffix}"
			"${ICONV_INCLUDE_DIR}/lib"
			"${ICONV_INCLUDE_DIR}/lib${_lib_suffix}"
		PATHS
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBICONV_INCLUDE_DIRS ${ICONV_INCLUDE_DIR} CACHE PATH "iconv include dir")
	set(LIBICONV_LIBRARIES ${ICONV_LIB} CACHE STRING "iconv libraries")

	find_package_handle_standard_args(Libiconv DEFAULT_MSG ICONV_LIB ICONV_INCLUDE_DIR)
	mark_as_advanced(ICONV_INCLUDE_DIR ICONV_LIB)
endif()
